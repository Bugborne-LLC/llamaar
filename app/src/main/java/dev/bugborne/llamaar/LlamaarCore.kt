package dev.bugborne.llamaar

class LlamaarCore {
    companion object {
        init {
            System.loadLibrary("llamaar")
        }
    }

    external fun initNative(nativeLibDir: String)
    external fun loadModelNative(modelPath: String): Boolean
    external fun generateTextNative(prompt: String): String
}