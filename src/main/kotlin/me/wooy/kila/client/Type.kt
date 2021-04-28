package me.wooy.kila.client

import java.lang.IllegalStateException

enum class Type(private val value: Int,val blockSize:Int) {
  MEMCACHED(1,1024*1024), REDIS(2,1024*1024);

  companion object {
    fun getById(value: Int) =
      when (value) {
        1 -> MEMCACHED
        2 -> REDIS
        else -> throw IllegalStateException()
      }

  }
}
