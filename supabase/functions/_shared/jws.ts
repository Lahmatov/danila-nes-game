// Pure JWS helpers with no dependencies — kept separate from appStore.ts so
// they can be unit-tested without pulling in jose/npm or network access.

/** JWS segments are base64url (uses "-" and "_"), which plain atob() rejects. */
export function decodeJWSPayload(jws: string): Record<string, unknown> {
  const segment = jws.split(".")[1] ?? "";
  const base64 = segment.replace(/-/g, "+").replace(/_/g, "/");
  const padded = base64.padEnd(base64.length + ((4 - (base64.length % 4)) % 4), "=");
  return JSON.parse(atob(padded));
}

/**
 * We don't verify the client-supplied JWS ourselves — we only peek at its
 * payload to get the transaction ID, then ask Apple's own server for the
 * authoritative record over TLS. That HTTPS response from Apple is the
 * actual verification; nothing the client sent is trusted directly.
 */
export function decodeUnverifiedTransactionId(jws: string): string | null {
  try {
    const payload = decodeJWSPayload(jws);
    return typeof payload.transactionId === "string" || typeof payload.transactionId === "number"
      ? String(payload.transactionId)
      : null;
  } catch {
    return null;
  }
}
