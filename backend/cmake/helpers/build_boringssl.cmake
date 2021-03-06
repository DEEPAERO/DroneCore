set(DRONECORESERVER_BORINGSSL_SOURCE_DIR "${DRONECORESERVER_EXTERNAL_SOURCE_DIR}/boringssl")
set(DRONECORESERVER_BORINGSSL_BINARY_DIR "${DRONECORESERVER_EXTERNAL_BINARY_DIR}/boringssl")

include(cmake/helpers/build_target.cmake)
build_target(${DRONECORESERVER_BORINGSSL_SOURCE_DIR} ${DRONECORESERVER_BORINGSSL_BINARY_DIR})

# Manually install boringssl
set(OPENSSL_ROOT_DIR ${DRONECORESERVER_BORINGSSL_BINARY_DIR}/install)

file(MAKE_DIRECTORY ${OPENSSL_ROOT_DIR})

file(COPY ${DRONECORESERVER_BORINGSSL_BINARY_DIR}/build/boringssl/src/boringssl/include/openssl DESTINATION ${OPENSSL_ROOT_DIR})
file(COPY ${DRONECORESERVER_BORINGSSL_BINARY_DIR}/build/boringssl/src/boringssl-build/ssl/libssl.a DESTINATION ${OPENSSL_ROOT_DIR}/lib)
file(COPY ${DRONECORESERVER_BORINGSSL_BINARY_DIR}/build/boringssl/src/boringssl-build/crypto/libcrypto.a DESTINATION ${OPENSSL_ROOT_DIR}/lib)
file(REMOVE ${OPENSSL_ROOT_DIR}/openssl/opensslv.h)
