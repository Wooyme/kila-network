package me.wooy.kila

import io.vertx.core.buffer.Buffer
import me.wooy.kila.client.BufferClient
import sun.misc.Queue
import java.lang.IndexOutOfBoundsException
import java.util.*
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.LinkedBlockingQueue
import kotlin.math.max

class CachedBuffer(private val maxBlock:Int = 4,private val maxSize:Int = 1024*1024,private val client:BufferClient) {
  inner class Block(var begin:Long, var end:Long, private val buffer:Buffer){
    fun between(offset:Long) = offset in begin..end
    fun write(offset:Long,buf:Buffer):Buffer?{
      val afterBegin = (offset-begin).toInt()
      val newEnd = offset+buf.length()
      if(newEnd>end){
        if(newEnd-begin>maxSize){
          buffer.setBuffer(afterBegin,buf,0,maxSize-(end-begin).toInt())
          release()
          return buf.getBuffer(maxSize-(end-begin).toInt(),buf.length())
        }else{
          buffer.setBuffer(afterBegin,buf)
          end = newEnd
        }
      }else{
        buffer.setBuffer(afterBegin,buf)
      }
      return null
    }

    fun release(){
      blocks.remove(this)
      client.send(begin,buffer)
    }
  }
  private val blocks = LinkedList<Block>()
  private val bufMap = ConcurrentHashMap<Long,Buffer>()
  private val releaseQueue = LinkedBlockingQueue<Long>()
  @Synchronized
  fun write(offset:Long,buffer: Buffer){
    bufMap.remove(offset)?.let {
      val delta = it.length()+buffer.length()-maxSize
      if(delta>0){
        val slice = buffer.getBuffer(0,it.length()-delta)
        releaseCache(offset-it.length()+slice.length(),it.appendBuffer(slice))
        bufMap[offset+buffer.length()] = buffer.getBuffer(it.length()-delta,it.length())
        releaseQueue.remove(offset)
        releaseQueue.put(offset+slice.length())
      }else{
        if(it.length()==maxSize){
          releaseCache(offset-it.length()+buffer.length(),it)
          return
        }
        bufMap[offset+buffer.length()] = it.appendBuffer(buffer)
        releaseQueue.remove(offset)
        releaseQueue.put(offset+buffer.length())
      }
    }?: run {
      if (isFull()) {
        releaseCache()
      }
      val delta = buffer.length()-maxSize
      if(delta>0){
        var tmpOffset = 0
        while(buffer.length()-tmpOffset<maxSize){
          releaseCache(offset+tmpOffset,buffer.getBuffer(tmpOffset,tmpOffset+ maxSize))
          tmpOffset+=maxSize
        }
        if(tmpOffset<buffer.length()) {
          bufMap[offset + buffer.length()] = buffer.getBuffer(tmpOffset, buffer.length())
          releaseQueue.put(offset+buffer.length())
        }
      }else{
        bufMap[offset+buffer.length()] = buffer
        releaseQueue.put(offset+buffer.length())
      }
    }
  }

  private fun isFull():Boolean{
    return bufMap.size>=maxBlock
  }

  private fun releaseCache(){
    val offset = releaseQueue.poll()
    val buf = bufMap.remove(offset)?:return
    client.send(offset-buf.length(),buf)
  }

  private fun releaseCache(offset:Long,buf:Buffer){
    client.send(offset,buf)
  }
}
