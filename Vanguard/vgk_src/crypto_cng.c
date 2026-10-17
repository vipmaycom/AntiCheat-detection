/*
 * crypto_cng.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Kernel CNG Cryptographic & Authenticode Engine
 *
 * Reconstructed from REA analysis of C:\Program Files\Riot Vanguard\vgk.sys:
 *   - RVA 0x156730..0x1567B0: L"SHA256", L"SHA1", L"SHA384", L"SHA512", L"ObjectLength", L"HashDigestLength"
 *   - RVA 0x156830..0x156930: PKCS#1 OIDs, L"Microsoft Primitive Provider", L"RSA", L"RSAPUBLICBLOB"
 *   - RVA 0x168260..0x16C1A8: 27-entry 600-byte (0x258) Trusted Root CAs Table in .data
 *                             (expanded from 16 in v1.17.12.4 to 27 in v1.19.1.18, adding "Gamerdoc Inc" @ 0x16BF50)
 */

#include "vgk.h"

/*
 * Complete 27-entry Trusted Root CA Whitelist extracted from .data RVA 0x168260..0x16BF50
 * Each record in vgk.sys v1.19.1.18 is 0x258 (600) bytes:
 *   +0x000: LIST_ENTRY ListEntry
 *   +0x010: UCHAR      EnabledFlag (0x00)
 *   +0x011: CHAR       SubjectName[64]
 *   +0x051: UCHAR      KeyType (1 = RSA-2048, 2 = RSA-4096)
 *   +0x052: UCHAR      Modulus[512]
 */
const PCSTR g_VgkTrustedRootCas[] = {
    "AAA Certificate Services",                                     /* #01 @ RVA 0x168260 (KeyType=1) */
    "Certum CA",                                                    /* #02 @ RVA 0x1684B8 (KeyType=1) */
    "Certum Trusted Network CA 2",                                  /* #03 @ RVA 0x168710 (KeyType=2) */
    "DigiCert Assured ID Root CA",                                  /* #04 @ RVA 0x168968 (KeyType=1) */
    "DigiCert CS RSA4096 Root G5",                                  /* #05 @ RVA 0x168BC0 (KeyType=2) */
    "DigiCert Global Root CA",                                      /* #06 @ RVA 0x168E18 (KeyType=1) */
    "DigiCert Global Root G2",                                      /* #07 @ RVA 0x169070 (KeyType=1) */
    "DigiCert High Assurance EV Root CA",                           /* #08 @ RVA 0x1692C8 (KeyType=1) */
    "DigiCert Trusted Root G4",                                     /* #09 @ RVA 0x169520 (KeyType=2) */
    "GlobalSign Root CA",                                           /* #10 @ RVA 0x169778 (KeyType=1) */
    "Sectigo Public Code Signing Root R46",                         /* #11 @ RVA 0x1699D0 (KeyType=2) */
    "GlobalSign",                                                   /* #12 @ RVA 0x169C28 (KeyType=1) */
    "VeriSign Universal Root Certification Authority",              /* #13 @ RVA 0x169E80 (KeyType=1) */
    "VeriSign Class 3 Public Primary Certification Authority - G5", /* #14 @ RVA 0x16A0D8 (KeyType=1) */
    "COMODO RSA Certification Authority",                           /* #15 @ RVA 0x16A330 (KeyType=2) */
    "DigiCert Trusted G4 Code Signing RSA4096 SHA384 2021 CA1",     /* #16 @ RVA 0x16A588 (KeyType=2) */
    "Entrust Root Certification Authority - G2",                    /* #17 @ RVA 0x16A7E0 (KeyType=1) */
    "Microsoft Root Certificate Authority 2010",                    /* #18 @ RVA 0x16AA38 (KeyType=2) */
    "Microsoft Development Root Certificate Authority 2014",        /* #19 @ RVA 0x16AC90 (KeyType=2) */
    "GlobalSign Code Signing Root R45",                             /* #20 @ RVA 0x16AEE8 (KeyType=2) */
    "DigiCert EV Code Signing CA",                                  /* #21 @ RVA 0x16B140 (KeyType=1) */
    "Microsoft Identity Verification Root Certificate Authority 2020", /* #22 @ RVA 0x16B398 (KeyType=2) */
    "thawte Primary Root CA",                                       /* #23 @ RVA 0x16B5F0 (KeyType=1) */
    "VeriSign Class 3 Code Signing 2010 CA",                        /* #24 @ RVA 0x16B848 (KeyType=1) */
    "Class 3 Public Primary Certification Authority",               /* #25 @ RVA 0x16BAA0 (KeyType=1) */
    "Certum Trusted Network CA",                                    /* #26 @ RVA 0x16BCF8 (KeyType=1) */
    "Gamerdoc Inc"                                                  /* #27 @ RVA 0x16BF50 (KeyType=2) */
};

const ULONG g_VgkTrustedRootCaCount = ARRAYSIZE(g_VgkTrustedRootCas);

/* =========================================================================
 * RECONSTRUCTED MULTI-HASH DISPATCHER (RVA 0x156730..0x1567B0)
 * Queries both L"ObjectLength" (0x156770) and L"HashDigestLength" (0x156790)
 * ========================================================================= */

NTSTATUS
VgkCngComputeDigestByType(
    _In_ VGK_CNG_HASH_TYPE HashType,
    _In_reads_bytes_(DataLength) PUCHAR Data,
    _In_ ULONG DataLength,
    _Out_writes_bytes_(DigestBufferSize) PUCHAR DigestOut,
    _In_ ULONG DigestBufferSize
)
{
    if (!Data || !DigestOut || DigestBufferSize == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    PCWSTR algName = NULL;
    ULONG  expectedDigestLen = 0;

    switch (HashType) {
    case VGK_HASH_SHA1:
        algName           = L"SHA1";    /* RVA 0x156740 */
        expectedDigestLen = 20;
        break;
    case VGK_HASH_SHA256:
        algName           = L"SHA256";  /* RVA 0x156730 */
        expectedDigestLen = 32;
        break;
    case VGK_HASH_SHA384:
        algName           = L"SHA384";  /* RVA 0x156750 */
        expectedDigestLen = 48;
        break;
    case VGK_HASH_SHA512:
        algName           = L"SHA512";  /* RVA 0x156760 */
        expectedDigestLen = 64;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if (DigestBufferSize < expectedDigestLen) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    BCRYPT_ALG_HANDLE  hAlg       = NULL;
    BCRYPT_HASH_HANDLE hHash      = NULL;
    PUCHAR             hashObject = NULL;
    ULONG              cbHashObj  = 0;
    ULONG              cbDigest   = 0;
    ULONG              cbResult   = 0;

    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hAlg,
        algName,
        VGK_MS_PRIMITIVE_PROVIDER_W, /* RVA 0x1568D0 */
        0
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = BCryptGetProperty(
        hAlg,
        VGK_OBJECT_LENGTH_W,         /* RVA 0x156770 */
        (PUCHAR)&cbHashObj,
        sizeof(ULONG),
        &cbResult,
        0
    );
    if (!NT_SUCCESS(status) || cbHashObj == 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return STATUS_UNSUCCESSFUL;
    }

    /* Also query HashDigestLength (RVA 0x156790 in v1.19.1.18) */
    BCryptGetProperty(
        hAlg,
        VGK_HASH_DIGEST_LENGTH_W,    /* RVA 0x156790 */
        (PUCHAR)&cbDigest,
        sizeof(ULONG),
        &cbResult,
        0
    );

    hashObject = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, cbHashObj, VGK_POOL_TAG);
    if (!hashObject) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = BCryptCreateHash(
        hAlg,
        &hHash,
        hashObject,
        cbHashObj,
        NULL,
        0,
        0
    );
    if (NT_SUCCESS(status)) {
        status = BCryptHashData(hHash, Data, DataLength, 0);
        if (NT_SUCCESS(status)) {
            status = BCryptFinishHash(hHash, DigestOut, expectedDigestLen, 0);
        }
        BCryptDestroyHash(hHash);
    }

    ExFreePoolWithTag(hashObject, VGK_POOL_TAG);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return status;
}

/* =========================================================================
 * RECONSTRUCTED RSA PKCS#1 SIGNATURE VERIFIER (RVA 0x156830..0x156930)
 * ========================================================================= */

NTSTATUS
VgkCngVerifyRsaSignature(
    _In_reads_bytes_(KeyBlobLength) PUCHAR RsaPublicBlob,
    _In_ ULONG KeyBlobLength,
    _In_z_ PCSTR OidString,
    _In_reads_bytes_(HashLength) PUCHAR HashDigest,
    _In_ ULONG HashLength,
    _In_reads_bytes_(SignatureLength) PUCHAR SignatureBytes,
    _In_ ULONG SignatureLength
)
{
    if (!RsaPublicBlob || !OidString || !HashDigest || !SignatureBytes) {
        return STATUS_INVALID_PARAMETER;
    }

    /* Map PKCS#1 OID string (RVA 0x156830..0x156890) to CNG hash identifier */
    BCRYPT_PKCS1_PADDING_INFO pkcs1Padding = { 0 };
    if (strcmp(OidString, VGK_OID_SHA1_RSA_A) == 0) {
        pkcs1Padding.pszAlgId = L"SHA1";
    } else if (strcmp(OidString, VGK_OID_SHA256_RSA_A) == 0) {
        pkcs1Padding.pszAlgId = L"SHA256";
    } else if (strcmp(OidString, VGK_OID_SHA384_RSA_A) == 0) {
        pkcs1Padding.pszAlgId = L"SHA384";
    } else if (strcmp(OidString, VGK_OID_SHA512_RSA_A) == 0) {
        pkcs1Padding.pszAlgId = L"SHA512";
    } else {
        return STATUS_NOT_SUPPORTED;
    }

    BCRYPT_ALG_HANDLE hRsaAlg = NULL;
    BCRYPT_KEY_HANDLE hKey    = NULL;

    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hRsaAlg,
        VGK_RSA_ALG_W,               /* RVA 0x156910 */
        VGK_MS_PRIMITIVE_PROVIDER_W, /* RVA 0x1568D0 */
        0
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = BCryptImportKeyPair(
        hRsaAlg,
        NULL,
        VGK_RSAPUBLICBLOB_W,         /* RVA 0x156920 */
        &hKey,
        RsaPublicBlob,
        KeyBlobLength,
        0
    );
    if (NT_SUCCESS(status)) {
        status = BCryptVerifySignature(
            hKey,
            &pkcs1Padding,
            HashDigest,
            HashLength,
            SignatureBytes,
            SignatureLength,
            BCRYPT_PAD_PKCS1
        );
        BCryptDestroyKey(hKey);
    }

    BCryptCloseAlgorithmProvider(hRsaAlg, 0);
    return status;
}

/* =========================================================================
 * ON-DISK FILE HEADER HASHING (IoCreateFileEx + ZwReadFile)
 * ========================================================================= */

NTSTATUS
VgkHashDiskFileHeader(
    _In_ PCWSTR NtFilePath,
    _Out_writes_bytes_(32) PUCHAR OutSha256
)
{
    RtlZeroMemory(OutSha256, 32);

    UNICODE_STRING filePath;
    RtlInitUnicodeString(&filePath, NtFilePath);

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(
        &objAttr,
        &filePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    HANDLE          fileHandle = NULL;
    IO_STATUS_BLOCK ioStatus   = { 0 };

    NTSTATUS status = IoCreateFileEx(
        &fileHandle,
        GENERIC_READ | SYNCHRONIZE,
        &objAttr,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0,
        CreateFileTypeNone,
        NULL,
        IO_NO_PARAMETER_CHECKING,
        NULL
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ULONG  readSize = 0x1000;
    PUCHAR buffer   = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, readSize, VGK_POOL_TAG);
    if (!buffer) {
        ZwClose(fileHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LARGE_INTEGER byteOffset = { 0 };
    status = ZwReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatus,
        buffer,
        readSize,
        &byteOffset,
        NULL
    );
    if (NT_SUCCESS(status) && ioStatus.Information > 0) {
        status = VgkCngComputeDigestByType(
            VGK_HASH_SHA256,
            buffer,
            (ULONG)ioStatus.Information,
            OutSha256,
            32
        );
    }

    ExFreePoolWithTag(buffer, VGK_POOL_TAG);
    ZwClose(fileHandle);
    return status;
}
