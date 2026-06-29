function Invoke-WindowsLocalTlsNative {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

function New-WindowsLocalTlsCertificateWithGo {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot,
        [Parameter(Mandatory = $true)]
        [string]$ServerCert,
        [Parameter(Mandatory = $true)]
        [string]$ServerKey
    )

    if (-not (Get-Command go -ErrorAction SilentlyContinue)) {
        throw "Neither openssl nor go was found. Run scripts/windows_setup_system.ps1 first, or create GoServer/keys/server.crt and GoServer/keys/server.key manually."
    }

    $certGenDir = Join-Path $RepoRoot ".tools\certgen"
    $certGenSource = Join-Path $certGenDir "local_tls_cert.go"

    if (-not (Test-Path $certGenDir)) {
        New-Item -ItemType Directory -Path $certGenDir | Out-Null
    }

    @'
package main

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"flag"
	"math/big"
	"net"
	"os"
	"time"
)

func must(err error) {
	if err != nil {
		panic(err)
	}
}

func main() {
	certPath := flag.String("cert", "", "certificate output path")
	keyPath := flag.String("key", "", "private key output path")
	flag.Parse()

	if *certPath == "" || *keyPath == "" {
		panic("cert and key output paths are required")
	}

	privateKey, err := rsa.GenerateKey(rand.Reader, 2048)
	must(err)

	serialLimit := new(big.Int).Lsh(big.NewInt(1), 128)
	serialNumber, err := rand.Int(rand.Reader, serialLimit)
	must(err)

	template := x509.Certificate{
		SerialNumber: serialNumber,
		Subject: pkix.Name{
			CommonName: "localhost",
		},
		NotBefore:             time.Now().Add(-time.Hour),
		NotAfter:              time.Now().Add(365 * 24 * time.Hour),
		KeyUsage:              x509.KeyUsageKeyEncipherment | x509.KeyUsageDigitalSignature,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		BasicConstraintsValid: true,
		DNSNames:              []string{"localhost"},
		IPAddresses:           []net.IP{net.ParseIP("127.0.0.1"), net.ParseIP("::1")},
	}

	certBytes, err := x509.CreateCertificate(rand.Reader, &template, &template, &privateKey.PublicKey, privateKey)
	must(err)

	certOut, err := os.Create(*certPath)
	must(err)
	defer certOut.Close()
	must(pem.Encode(certOut, &pem.Block{Type: "CERTIFICATE", Bytes: certBytes}))

	keyOut, err := os.OpenFile(*keyPath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0600)
	must(err)
	defer keyOut.Close()
	must(pem.Encode(keyOut, &pem.Block{Type: "RSA PRIVATE KEY", Bytes: x509.MarshalPKCS1PrivateKey(privateKey)}))
}
'@ | Set-Content -Path $certGenSource -Encoding UTF8

    Write-Host "Generating local TLS certificate with Go"
    Invoke-WindowsLocalTlsNative go run $certGenSource -cert $ServerCert -key $ServerKey
}

function Ensure-WindowsLocalTlsFiles {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot
    )

    $goServerDir = Join-Path $RepoRoot "GoServer"
    $cppClientDir = Join-Path $RepoRoot "CPPClient"
    $keysDir = Join-Path $goServerDir "keys"
    $serverKey = Join-Path $keysDir "server.key"
    $serverCert = Join-Path $keysDir "server.crt"
    $clientCert = Join-Path $cppClientDir "server.crt"

    if (-not (Test-Path $keysDir)) {
        New-Item -ItemType Directory -Path $keysDir | Out-Null
    }

    if ((-not (Test-Path $serverKey)) -or (-not (Test-Path $serverCert))) {
        if (Get-Command openssl -ErrorAction SilentlyContinue) {
            Write-Host "Generating local TLS certificate with OpenSSL"
            Invoke-WindowsLocalTlsNative openssl req -x509 -newkey rsa:2048 -nodes `
                -keyout $serverKey `
                -out $serverCert `
                -days 365 `
                -subj "/CN=localhost" `
                -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"
        } else {
            New-WindowsLocalTlsCertificateWithGo -RepoRoot $RepoRoot -ServerCert $serverCert -ServerKey $serverKey
        }
    }

    if (Test-Path $serverCert) {
        Copy-Item -Path $serverCert -Destination $clientCert -Force
    }
}
