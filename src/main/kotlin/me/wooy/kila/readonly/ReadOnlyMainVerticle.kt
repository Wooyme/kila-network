package me.wooy.kila.readonly

import io.vertx.core.AbstractVerticle
import io.vertx.core.buffer.Buffer
import io.vertx.core.net.NetSocket
import io.vertx.ext.web.client.WebClient
import io.vertx.jdbcclient.JDBCConnectOptions
import io.vertx.jdbcclient.JDBCPool
import io.vertx.sqlclient.PoolOptions
import me.wooy.kila.readonly.client.BaiduPCSClient
import me.wooy.kila.utils.appendCLong
import me.wooy.kila.utils.getCLong
import java.util.concurrent.ConcurrentHashMap

class ReadOnlyMainVerticle:AbstractVerticle() {
  private val cachedClient = ConcurrentHashMap<String,BaiduPCSClient>()
  private val jdbcPool by lazy {
    JDBCPool.pool(vertx, JDBCConnectOptions().setJdbcUrl("jdbc:sqlite:kila.db"), PoolOptions().setMaxSize(1))
  }

  private val webClient by lazy {WebClient.create(vertx)}
  override fun start() {
    vertx.createNetServer().connectHandler { conn ->
      println("Connect")
      conn.handler {
        when (it.getByte(0)) {
          0.toByte() -> {
            handleList(conn)
          }
          2.toByte() -> {
            val uuid = it.getString(1, 37)
            val offset = it.getCLong(37)
            val size = it.getCLong(37 + Long.SIZE_BYTES).toInt()
            handleRead(conn,uuid, offset, size)
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

  private fun handleList(conn: NetSocket){
    jdbcPool.preparedQuery("SELECT * FROM files").execute().onSuccess {
      val buffer = Buffer.buffer()
      it.forEach {
        val uuid = it.getString("uuid")
        val path = it.getString("path")
        val size = it.getLong("size")
        val zeros = ByteArray(1024-path.length){0}
        buffer.appendString(uuid).appendCLong(size).appendString(path).appendBytes(zeros)
      }
      conn.write(buffer)
    }
  }

  private fun handleRead(conn:NetSocket,uuid: String, offset: Long, size: Int) {
    println("[Read] uuid:$uuid offset:$offset size:$size")
    val client = cachedClient[uuid]?: kotlin.run {
      val baiduPCSClient = BaiduPCSClient(uuid,webClient)
      cachedClient[uuid] = baiduPCSClient
      baiduPCSClient
    }
    client.read(offset,size).onSuccess {
      conn.write(it)
    }.onFailure {
      conn.write(Buffer.buffer())
    }
  }
}
