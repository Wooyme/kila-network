package me.wooy.kila.basic

import me.wooy.kila.client.Type
import java.util.concurrent.atomic.AtomicLong

data class FileDto(val uuid:String,val size:AtomicLong, val type: Type)
