package me.wooy.kila.readonly.client

import io.vertx.core.*
import io.vertx.core.buffer.Buffer
import io.vertx.ext.web.client.HttpResponse
import io.vertx.ext.web.client.WebClient
import org.apache.commons.io.IOUtils
import java.lang.Exception
import java.nio.charset.Charset
import java.util.concurrent.ConcurrentHashMap


class BaiduPCSClient(private val uuid:String,private val webClient: WebClient) {
  private val urlMap = ConcurrentHashMap<Int,Triple<String,Long,Long>>()
  private fun getURL(offset:Long):Triple<String,Long,Long>{
    val index = (offset/ SINGLE_SIZE).toInt()
    urlMap[index]?.let { return it }
    val p = ProcessBuilder("BaiduPCS-Go", "locate",ROOT+uuid+index).start()
    val stderr: String = IOUtils.toString(p.errorStream, Charset.defaultCharset())
    val stdout: String = IOUtils.toString(p.inputStream, Charset.defaultCharset())
    if(stderr.isNotEmpty()){
      println(stderr)
    }
    println(stdout)
    val url = Regex("(https?|ftp|file)://[-a-zA-Z0-9+&@#/%?=~_|!:,.;]*[-a-zA-Z0-9+&@#/%=~_|]").find(stdout)?.value?:throw Exception("[$uuid$index] Url not found")
    val result = Triple(url,index * SINGLE_SIZE,(index+1) * SINGLE_SIZE)
    urlMap[index] = result
    return result
  }
  fun read(offset:Long,size:Int):Future<Buffer>{
    val tail = offset+size-1
    val promise = Promise.promise<Buffer>()
    val (url,begin,end) = getURL(offset)
    val fut = if(offset+size<=end) {
      CompositeFuture.all(listOf(webClient.getAbs(url)
        .putHeader("User-Agent","netdisk;2.2.51.6;netdisk;10.0.63;PC;android-android")
        .putHeader("Range","bytes=${offset-begin}-${tail-begin}")
        .send())
      )
    }else{
      val fut1 = webClient.getAbs(url)
        .putHeader("User-Agent","netdisk;2.2.51.6;netdisk;10.0.63;PC;android-android")
        .putHeader("Range","bytes=${offset-begin}-${end-1}")
        .send()
      val (nextUrl,nextBegin,nextEnd) = getURL(end)
      val fut2 = webClient.getAbs(nextUrl)
        .putHeader("User-Agent","netdisk;2.2.51.6;netdisk;10.0.63;PC;android-android")
        .putHeader("Range","bytes=0-${tail-nextBegin}")
        .send()
      CompositeFuture.all(fut1,fut2)
    }
    fut.onComplete {
      val buffer = it.result().list<HttpResponse<Buffer>>().map { it.body() }.reduce { acc, buffer -> acc.appendBuffer(buffer) }
      if(buffer.length()!=size){
        println("Response size [${buffer.length()}] doesn't match the required [${size}], clear url cache and retry")
        urlMap.clear()
        read(offset,size).onSuccess {
          promise.complete(it)
        }.onFailure {
          promise.fail("Response size [${buffer.length()}] doesn't match the required [${size}]")
        }
      }else {
        promise.complete(buffer)
      }
    }
    return promise.future()
  }

  companion object{
    private const val SINGLE_SIZE = 1000*1024L
    private const val ROOT = "/kila/"
  }
}
