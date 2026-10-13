#include "content/WindowsTls.h"
#include <windows.h>
#include <wincrypt.h>
#undef X509_NAME
#undef X509_EXTENSIONS
#undef PKCS7_SIGNER_INFO
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace qliteview {
namespace {
CURLcode importSystemRoots(CURL*, void* context, void*)
{
    HCERTSTORE roots = CertOpenSystemStoreW(0, L"ROOT");
    if (!roots) return CURLE_SSL_CACERT_BADFILE;
    X509_STORE* store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(context));
    PCCERT_CONTEXT certificate = nullptr;
    while ((certificate = CertEnumCertificatesInStore(roots, certificate))) {
        const unsigned char* encoded = certificate->pbCertEncoded;
        X509* x509 = d2i_X509(nullptr, &encoded, certificate->cbCertEncoded);
        if (x509) {
            X509_STORE_add_cert(store, x509);
            X509_free(x509);
        }
        ERR_clear_error(); // A duplicate store entry is harmless.
    }
    CertCloseStore(roots, 0);
    return CURLE_OK;
}
}
void configureTls(CURL* curl)
{
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, importSystemRoots);
}
}
