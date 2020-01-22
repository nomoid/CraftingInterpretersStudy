package com.nomoid.jlox;

interface LoxClasslike {
    LoxFunction findMethod(String name);
    LoxFunction findGetter(String name);
    String name();
}