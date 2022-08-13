package somepackage;

import com.sun.org.apache.xerces.internal.impl.dv.util.HexBin;
import org.bytedeco.javacpp.Loader;
import org.bytedeco.javacpp.annotation.Cast;
import org.bytedeco.javacpp.annotation.Platform;

import java.nio.ByteBuffer;

@Platform(include = "somepackage/Sm4Ref.h")
public class Sm4Ref {

    static {
        Loader.load(Sm4Ref.class);
    }

    public static native void sm4_key_schedule(@Cast("const uint8_t*")byte[] key, @Cast("uint32_t*") int[] rk);
    public static native void sm4_encrypt(@Cast("const uint32_t*") int[] rk, @Cast("const uint8_t*")byte[] plaintext, @Cast("uint8_t*")byte[] ciphertext);

    public static native void sm4_decrypt(@Cast("const uint32_t*") int[] rk, @Cast("const uint8_t*")byte[] ciphertext, @Cast("uint8_t*")byte[] plaintext);

    public static native void sm4_encrypt4(@Cast("const uint32_t*") int[] rk, @Cast("uint32_t*")int[] plaintext, @Cast("const uint32_t*")int[] ciphertext);

    public static void main(String[] args) throws Exception {

        byte[] key = HexBin.decode("0123456789ABCDEFFEDCBA9876543210");
        int[] rk = new int[32];
        sm4_key_schedule(key, rk);

        byte[] plainText = HexBin.decode("681EDF34D206965E86B3E94F536E4246");
        byte[] cipherText = new byte[16];

        byte[] decrypt = new byte[16];


        int[] plainTex2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
        int[] cipherText2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x11};

        sm4_encrypt(rk, plainText, cipherText);
        System.out.println(HexBin.encode(cipherText));

        sm4_decrypt(rk, cipherText, decrypt);
        System.out.println(HexBin.encode(decrypt));

        sm4_encrypt4(rk, plainTex2, cipherText2);

        ByteBuffer buffer = ByteBuffer.allocate(16 * 4);
        for (int i : cipherText2) {
            buffer.putInt(i);
        }
        System.out.println(HexBin.encode(buffer.array()));
    }
}
