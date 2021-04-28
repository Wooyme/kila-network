package me.wooy.kila

import io.vertx.core.AbstractVerticle
import io.vertx.core.Promise
import io.vertx.core.buffer.Buffer
import io.vertx.core.net.NetSocket
import io.vertx.jdbcclient.JDBCConnectOptions
import io.vertx.jdbcclient.JDBCPool
import io.vertx.sqlclient.PoolOptions
import io.vertx.sqlclient.Tuple
import me.wooy.kila.basic.FileDto
import me.wooy.kila.client.Type
import me.wooy.kila.utils.appendCInt
import me.wooy.kila.utils.appendCLong
import me.wooy.kila.utils.getCInt
import me.wooy.kila.utils.getCLong
import java.lang.NullPointerException
import java.util.concurrent.ConcurrentHashMap

class MainVerticle : AbstractVerticle() {
  private val fileMap = ConcurrentHashMap<String,FileDto>()
  private val bufferMap = ConcurrentHashMap<Pair<String,Long>,Buffer>()
  private val jdbcPool by lazy {
    JDBCPool.pool(vertx, JDBCConnectOptions().setJdbcUrl("jdbc:sqlite:sample.db"), PoolOptions().setMaxSize(1))
  }

  override fun start(startPromise: Promise<Void>) {
    vertx.createNetServer().connectHandler { conn ->
      println("Connect")
      conn.handler {
        when (it.getByte(0)) {
          0.toByte() -> {

          }
          1.toByte() -> {
            val uuid = it.getString(1, 37)
            val pathLen = it.getCLong(37)
            val path = it.getString(37 + Long.SIZE_BYTES, 37 + Long.SIZE_BYTES + pathLen.toInt())
            handleCreate(conn,uuid, path)
          }
          2.toByte() -> {
            val uuid = it.getString(1, 37)
            val offset = it.getCLong(37)
            val size = it.getCLong(37 + Long.SIZE_BYTES)
            val buf = Buffer.buffer()
            handleRead(uuid, offset, size, buf)
            conn.write(buf)
          }
          3.toByte() -> {
            val uuid = it.getString(1, 37)
            val offset = it.getCLong(37)
            val size = it.getCLong(37 + Long.SIZE_BYTES)
            val buf = it.getBuffer(37 + Long.SIZE_BYTES + Long.SIZE_BYTES, 37 + Long.SIZE_BYTES + Long.SIZE_BYTES + size.toInt())
            handleWrite(conn,uuid, offset, size, buf)
          }
        }
      }.closeHandler {
        println("Close")
      }.exceptionHandler {
        it.printStackTrace()
      }
    }.listen(2325) {
      println("Listen at 2325")
    }
  }

  private fun handleCreate(conn: NetSocket, uuid: String, path: String) {
    println("[Create] uuid:${uuid} path:${path}")
    jdbcPool.preparedQuery("""INSERT INTO "files" (uuid,path,"size") values (?,?,0)""")
      .execute(Tuple.of(uuid, path))
      .onFailure {
        conn.write(Buffer.buffer().appendCInt(1))
      }.onSuccess {
        conn.write(Buffer.buffer().appendCInt(0))
      }
  }

  private fun handleWrite(conn:NetSocket,uuid: String, offset: Long, size: Long, buf: Buffer) {
    println("[Write] uuid:${uuid} offset:$offset size:$size")
    val file = fileMap[uuid]?:throw NullPointerException()
    val oldSize = file.size.get()
    val type = file.type
    val newSize = if(offset+size-oldSize>oldSize){
      offset+size
    }else{
      size
    }
    val block:Long = offset/type.blockSize
    val blockOffset = offset - block*type.blockSize
    val buffer = bufferMap[uuid to block]?.let {
      val buffer = Buffer.buffer()
      bufferMap[uuid to block] = buffer
      buffer
    }
    if(blockOffset==0L){

    }else{

    }
    file.size.addAndGet(newSize)
  }

  private fun handleRead(uuid: String, offset: Long, size: Long, buf: Buffer) {
    println("[Read] uuid:$uuid offset:$offset size:$size")
    buf.appendString("kila").appendByte(0)
  }
}
