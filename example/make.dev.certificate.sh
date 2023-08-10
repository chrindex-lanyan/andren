#!/bin/sh

PRI_KEY="server_private.key"
PUB_KEY="server_public.key"
CER_CSR="server_certificate.csr"
CER_CRT="server_certificate.crt"

echo "##### Server #####"
echo "Private Key = ${PRI_KEY}"
echo "Public Key = ${PUB_KEY}"
echo "Certificate CSR = ${CER_CSR}"
echo "Certificate CRT = ${CER_CRT}"


# 生成私钥文件
openssl genpkey -algorithm RSA -out ${PRI_KEY}

# 生成公钥文件：
openssl rsa -pubout -in ${PRI_KEY} -out ${PUB_KEY}

# 生成自签名的证书请求：
openssl req -new -key ${PRI_KEY} -out ${CER_CSR}

# 使用私钥和证书请求生成自签名的证书：
openssl x509 -req -days 365 -in ${CER_CSR} -signkey ${PRI_KEY} -out ${CER_CRT}


PRI_KEY="client_private.key"
PUB_KEY="client_public.key"
CER_CSR="client_certificate.csr"
CER_CRT="client_certificate.crt"

echo "##### Client #####"
echo "Private Key = ${PRI_KEY}"
echo "Public Key = ${PUB_KEY}"
echo "Certificate CSR = ${CER_CSR}"
echo "Certificate CRT = ${CER_CRT}"



# 生成私钥文件
openssl genpkey -algorithm RSA -out ${PRI_KEY}

# 生成公钥文件：
openssl rsa -pubout -in ${PRI_KEY} -out ${PUB_KEY}

# 生成自签名的证书请求：
openssl req -new -key ${PRI_KEY} -out ${CER_CSR}

# 使用私钥和证书请求生成自签名的证书：
openssl x509 -req -days 365 -in ${CER_CSR} -signkey ${PRI_KEY} -out ${CER_CRT}


