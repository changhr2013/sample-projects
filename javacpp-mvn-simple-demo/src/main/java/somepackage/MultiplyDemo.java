package somepackage;

import org.bytedeco.javacpp.*;
import org.bytedeco.javacpp.annotation.*;

@Platform(include="somepackage/MultiplyDemo.h")
public class MultiplyDemo {

    static {
        Loader.load();
    }

    public static native int multiply(int a, int b);

    public static native void selectionSort(int[] arr, int n);

    public static native void hello();

    public static void main(String[] args) {
        System.out.println("multiply(123,100): " + multiply(123,100));
    }
    
}