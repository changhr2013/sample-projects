package somepackage;

import org.bytedeco.javacpp.*;
import org.bytedeco.javacpp.annotation.*;

@Platform(include="somepackage/Abc.h")
@Namespace("Abc")
public class Abc {
    public static class MyFunc extends Pointer {
        static { Loader.load(); }
        public MyFunc() { allocate(); }
        private native void allocate();

        // to call add functions
        public native int add(int a, int b);
    }

    public static void main(String[] args) {
        MyFunc myFunc = new MyFunc();
        System.out.println(myFunc .add(111,222));
    }
}