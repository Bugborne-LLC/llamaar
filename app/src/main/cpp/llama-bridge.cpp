#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include "llama.h"

#define LOG_TAG "Llamaar"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static llama_model * g_model = nullptr;
static llama_context * g_context = nullptr;
static llama_sampler * g_sampler = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_dev_bugborne_llamaar_LlamaarCore_initNative(JNIEnv *env, jobject, jstring nativeLibDir) {
    const char *path = env->GetStringUTFChars(nativeLibDir, 0);
    LOGI("Loading backends from %s", path);
    ggml_backend_load_all_from_path(path);
    env->ReleaseStringUTFChars(nativeLibDir, path);

    llama_backend_init();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_dev_bugborne_llamaar_LlamaarCore_loadModelNative(JNIEnv *env, jobject, jstring modelPath) {
    const char *path = env->GetStringUTFChars(modelPath, 0);
    LOGI("Loading model from: %s", path);

    llama_model_params model_params = llama_model_default_params();
    g_model = llama_model_load_from_file(path, model_params);
    env->ReleaseStringUTFChars(modelPath, path);

    if (!g_model) {
        LOGE("Failed to load model!");
        return false;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;
    ctx_params.n_threads = 4;
    ctx_params.n_threads_batch = 4;
    g_context = llama_init_from_model(g_model, ctx_params);

    if (!g_context) {
        LOGE("Failed to create context!");
        return false;
    }

    g_sampler = llama_sampler_init_greedy();
    LOGI("Model loaded successfully!");
    return true;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_dev_bugborne_llamaar_LlamaarCore_generateTextNative(JNIEnv *env, jobject, jstring prompt) {
    if (!g_context || !g_model || !g_sampler) {
        return env->NewStringUTF("[Error: Model not loaded]");
    }

    const char *text = env->GetStringUTFChars(prompt, 0);
    std::string input(text);
    env->ReleaseStringUTFChars(prompt, text);

    // Get the vocab from the model (Required by the newest llama.cpp API)
    const llama_vocab * vocab = llama_model_get_vocab(g_model);

    std::vector<llama_token> tokens(input.size() + 1);
    int n_tokens = llama_tokenize(vocab, input.c_str(), input.size(), tokens.data(), tokens.size(), false, true);
    tokens.resize(n_tokens);

    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    if (llama_decode(g_context, batch) != 0) {
        LOGE("Failed to evaluate prompt!");
        return env->NewStringUTF("[Error: Decode failed]");
    }

    std::string result = input + "\n\n";
    char buf[128];
    for (int i = 0; i < 64; i++) {
        llama_token new_token_id = llama_sampler_sample(g_sampler, g_context, -1);

        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break;
        }

        // Pass the vocab here as well
        int n_chars = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
        if (n_chars > 0) {
            result += std::string(buf, n_chars);
        }

        batch = llama_batch_get_one(&new_token_id, 1);
        if (llama_decode(g_context, batch) != 0) {
            break;
        }
    }

    return env->NewStringUTF(result.c_str());
}