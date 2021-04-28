package me.wooy.kila.utils

import io.vertx.core.buffer.Buffer

val bigEnding = System.getProperty("bigEnding")!!.toBoolean()
fun Buffer.getCInt(pos: Int) =
  if (bigEnding) {
    this.getInt(pos)
  } else {
    this.getIntLE(pos)
  }

fun Buffer.getCLong(pos: Int) =
  if (bigEnding) {
    this.getLong(pos)
  } else {
    this.getLongLE(pos)
  }

fun Buffer.appendCInt(int: Int):Buffer =
  if (bigEnding) {
    this.appendInt(int)
  } else {
    this.appendIntLE(int)
  }


fun Buffer.appendCLong(long: Long): Buffer =
  if (bigEnding) {
    appendLong(long)
  } else {
    appendLongLE(long)
  }
