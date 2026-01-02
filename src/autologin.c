#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <time.h>

#define MAX_RESPONSE_SIZE 4096

struct MemoryStruct {
    char *memory;
    size_t size;
};

typedef struct {
    char username[64];
    char password[64];
    char domain[16];
    char auth_server[64];
    char ac_id[8];
    int use_https;
} Config;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

static void int_overflow(int *val) {
    const int maxint = 2147483647;
    if (*val > maxint || *val < -maxint-1) {
        *val = (*val + (maxint + 1)) % (2 * (maxint + 1)) - maxint - 1;
    }
}

static unsigned int unsigned_right_shift(int n, int b) {
    if (n < 0) {
        n = (n & 0x7fffffff) + 0x80000000;
    }
    return (unsigned int)n >> b;
}

static int str_to_long_arr(const char *string, int **result, int include_length) {
    int length = strlen(string);
    int arr_len = (length + 3) / 4;
    if (include_length) arr_len++;
    
    *result = calloc(arr_len, sizeof(int));
    if (!*result) return 0;
    
    for (int i = 0; i < length; i += 4) {
        int val = 0;
        for (int j = 0; j < 4 && i + j < length; j++) {
            val |= ((unsigned char)string[i + j]) << (j * 8);
        }
        (*result)[i / 4] = val;
    }
    
    if (include_length) {
        (*result)[arr_len - 1] = length;
    }
    
    return arr_len;
}

static char* xxtea_encrypt(const char *plain_text, const char *key) {
    if (!plain_text || !*plain_text) return strdup("");
    
    int *v, *k;
    int v_len = str_to_long_arr(plain_text, &v, 1);
    int k_len = str_to_long_arr(key, &k, 0);
    
    if (k_len < 4) {
        k = realloc(k, 4 * sizeof(int));
        for (int i = k_len; i < 4; i++) k[i] = 0;
    }
    
    int n = v_len - 1;
    int z = v[n], y = v[0];
    int delta = 0x9e3779b9;
    int q = (6 + 52) / (n + 1);
    int sum = 0;
    
    while (q-- > 0) {
        sum += delta;
        int_overflow(&sum);
        int e = unsigned_right_shift(sum, 2) & 3;
        
        for (int p = 0; p < n; p++) {
            y = v[p + 1];
            int mx = (unsigned_right_shift(z, 5) ^ (y << 2)) + 
                     ((unsigned_right_shift(y, 3) ^ (z << 4)) ^ (sum ^ y)) + 
                     (k[(p & 3) ^ e] ^ z);
            int_overflow(&mx);
            z = v[p] += mx;
            int_overflow(&z);
        }
        
        y = v[0];
        int mx = (unsigned_right_shift(z, 5) ^ (y << 2)) + 
                 ((unsigned_right_shift(y, 3) ^ (z << 4)) ^ (sum ^ y)) + 
                 (k[(n & 3) ^ e] ^ z);
        int_overflow(&mx);
        z = v[n] += mx;
        int_overflow(&z);
    }
    
    char *result = malloc((v_len - 1) * 4 + 1);
    for (int i = 0; i < (v_len - 1) * 4; i++) {
        result[i] = (v[i / 4] >> ((i % 4) * 8)) & 0xff;
    }
    result[(v_len - 1) * 4] = '\0';
    
    free(v);
    free(k);
    return result;
}

static void hmac_md5(const char *key, const char *data, char *output) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    unsigned int len;
    
    HMAC(EVP_md5(), key, strlen(key), (unsigned char*)data, strlen(data), digest, &len);
    
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(output + i * 2, "%02x", digest[i]);
    }
    output[32] = '\0';
}

static void sha1_hash(const char *data, char *output) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)data, strlen(data), digest);
    
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(output + i * 2, "%02x", digest[i]);
    }
    output[40] = '\0';
}

static char* base64_encode_custom(const char *data, int len) {
    const char *custom_alpha = "LVoJPiCN2R8G90yg+hmFHuacZ1OWMnrsSTXkYpUq/3dlbfKwv6xztjI7DeBE45QA";
    const char *std_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int out_len = 4 * ((len + 2) / 3);
    char *encoded = malloc(out_len + 1);
    if (!encoded) return NULL;
    
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    
    while (len--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++) {
                encoded[j++] = std_alpha[char_array_4[i]];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (int k = i; k < 3; k++) char_array_3[k] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (int k = 0; k < i + 1; k++) {
            encoded[j++] = std_alpha[char_array_4[k]];
        }
        
        while (i++ < 3) encoded[j++] = '=';
    }
    
    encoded[j] = '\0';
    
    for (int k = 0; k < j; k++) {
        if (encoded[k] != '=') {
            char *pos = strchr(std_alpha, encoded[k]);
            if (pos) {
                encoded[k] = custom_alpha[pos - std_alpha];
            }
        }
    }
    
    char *result = malloc(j + 8);
    strcpy(result, "{SRBX1}");
    strcat(result, encoded);
    free(encoded);
    
    return result;
}

static char* get_token(const Config *config) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    char url[512];
    char *token = NULL;
    
    curl = curl_easy_init();
    if (!curl) return NULL;
    
    snprintf(url, sizeof(url), "%s://%s/cgi-bin/get_challenge?callback=jQuery&username=%s%s&ip=&_=%ld",
             config->use_https ? "https" : "http", config->auth_server,
             config->username, config->domain, time(NULL) * 1000);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && chunk.memory) {
        char *start = strstr(chunk.memory, "\"challenge\":\"");
        if (start) {
            start += 13;
            char *end = strchr(start, '"');
            if (end) {
                int len = end - start;
                token = malloc(len + 1);
                strncpy(token, start, len);
                token[len] = '\0';
            }
        }
    }
    
    if (chunk.memory) free(chunk.memory);
    return token;
}

static int do_login(const Config *config) {
    char *token = get_token(config);
    if (!token) return 0;
    
    char password_hmd5[33];
    hmac_md5(token, config->password, password_hmd5);
    
    char user_info[512];
    snprintf(user_info, sizeof(user_info),
             "{\"username\":\"%s%s\",\"password\":\"%s\",\"ip\":\"\",\"acid\":\"%s\",\"enc_ver\":\"srun_bx1\"}",
             config->username, config->domain, config->password, config->ac_id);
    
    char *encrypted = xxtea_encrypt(user_info, token);
    if (!encrypted) {
        free(token);
        return 0;
    }
    
    char *info_encoded = base64_encode_custom(encrypted, strlen(encrypted));
    free(encrypted);
    
    char chkstr[2048];
    snprintf(chkstr, sizeof(chkstr), "%s%s%s%s%s%s%s%s%s%s200%s1%s%s",
             token, config->username, config->domain,
             token, password_hmd5,
             token, config->ac_id,
             token, "",
             token, token, token, info_encoded);
    
    char chksum[41];
    sha1_hash(chkstr, chksum);
    
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    char url[1024];
    
    curl = curl_easy_init();
    if (!curl) {
        free(token);
        free(info_encoded);
        return 0;
    }
    
    snprintf(url, sizeof(url),
             "%s://%s/cgi-bin/srun_portal?callback=jQuery&action=login&username=%s%s&password={MD5}%s&os=Linux&name=Linux&double_stack=0&chksum=%s&info=%s&ac_id=%s&ip=&n=200&type=1&_=%ld",
             config->use_https ? "https" : "http", config->auth_server,
             config->username, config->domain, password_hmd5, chksum, info_encoded, config->ac_id, time(NULL) * 1000);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    int success = 0;
    if (res == CURLE_OK && chunk.memory) {
        if (strstr(chunk.memory, "\"error\":\"ok\"") || strstr(chunk.memory, "login_ok")) {
            success = 1;
        }
    }
    
    free(token);
    free(info_encoded);
    if (chunk.memory) free(chunk.memory);
    
    return success;
}

int main(int argc, char *argv[]) {
    Config config = {0};
    
    if (argc < 5) {
        printf("Usage: %s <username> <password> <domain> <server>\n", argv[0]);
        return 1;
    }
    
    strncpy(config.username, argv[1], sizeof(config.username) - 1);
    strncpy(config.password, argv[2], sizeof(config.password) - 1);
    strncpy(config.domain, argv[3], sizeof(config.domain) - 1);
    strncpy(config.auth_server, argv[4], sizeof(config.auth_server) - 1);
    strcpy(config.ac_id, "1");
    config.use_https = 0;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    int result = do_login(&config);
    curl_global_cleanup();
    
    return result ? 0 : 1;
}