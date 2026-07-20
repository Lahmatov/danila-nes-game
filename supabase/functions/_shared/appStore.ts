import { SignJWT, importPKCS8 } from "npm:jose@5";
import { decodeJWSPayload } from "./jws.ts";

export { decodeUnverifiedTransactionId } from "./jws.ts";

export interface AppStoreTransactionInfo {
  productId: string;
  expiresDate?: number;
  revocationDate?: number;
}

export async function fetchTransactionFromAppStore(
  transactionId: string
): Promise<AppStoreTransactionInfo> {
  const token = await signAppStoreServerJWT();

  for (const base of [
    "https://api.storekit.itunes.apple.com",
    "https://api.storekit-sandbox.itunes.apple.com",
  ]) {
    const response = await fetch(`${base}/inApps/v1/transactions/${transactionId}`, {
      headers: { Authorization: `Bearer ${token}` },
    });

    if (response.status === 404) continue; // wrong environment, try the other one

    if (!response.ok) {
      throw new Error(`App Store API ${response.status}: ${await response.text()}`);
    }

    const data = await response.json();
    const payload = decodeJWSPayload(data.signedTransactionInfo);
    return {
      productId: String(payload.productId),
      expiresDate: payload.expiresDate as number | undefined,
      revocationDate: payload.revocationDate as number | undefined,
    };
  }

  throw new Error("Transaction not found in production or sandbox App Store environment");
}

async function signAppStoreServerJWT(): Promise<string> {
  const privateKeyPEM = Deno.env.get("APPLE_IAP_PRIVATE_KEY")!;
  const keyId = Deno.env.get("APPLE_IAP_KEY_ID")!;
  const issuerId = Deno.env.get("APPLE_IAP_ISSUER_ID")!;
  const bundleId = Deno.env.get("APPLE_BUNDLE_ID")!;

  const privateKey = await importPKCS8(privateKeyPEM, "ES256");

  return await new SignJWT({ bid: bundleId })
    .setProtectedHeader({ alg: "ES256", kid: keyId, typ: "JWT" })
    .setIssuer(issuerId)
    .setIssuedAt()
    .setExpirationTime("15m")
    .setAudience("appstoreconnect-v1")
    .sign(privateKey);
}
