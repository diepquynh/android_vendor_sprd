package com.sprd.validationtools.itemstest;

public class TupleIII<A, B, C> extends Tuple<A, B> {

    public final C third;

    public TupleIII(A first, B second, C third) {
        super(first, second);
        this.third = third;
    }
}
