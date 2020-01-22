package com.nomoid.jlox;

interface LoxClasslike {
    LoxFunction findMethod(String name);
    String name();
}