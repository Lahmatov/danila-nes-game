import { decodeJWSPayload, decodeUnverifiedTransactionId } from "./jws.ts";

// Local deep-equal assert instead of jsr:@std/assert so the tests run with no
// network access at all: `deno test supabase/functions/_shared/jws_test.ts`.
function assertEquals(actual: unknown, expected: unknown) {
  const a = JSON.stringify(actual);
  const b = JSON.stringify(expected);
  if (a !== b) throw new Error(`assertEquals failed:\n  actual:   ${a}\n  expected: ${b}`);
}

function base64url(value: unknown): string {
  return btoa(JSON.stringify(value))
    .replace(/\+/g, "-")
    .replace(/\//g, "_")
    .replace(/=+$/, "");
}

function fakeJWS(payload: unknown): string {
  return `${base64url({ alg: "ES256" })}.${base64url(payload)}.fake-signature`;
}

Deno.test("decodes a plain payload", () => {
  const jws = fakeJWS({ transactionId: "2000000123456789", productId: "x" });
  assertEquals(decodeUnverifiedTransactionId(jws), "2000000123456789");
});

Deno.test("decodes payloads whose base64url form contains - and _", () => {
  // These byte sequences produce "+" and "/" in plain base64, which become
  // "-" and "_" in base64url — exactly the case plain atob() chokes on.
  const payload = { transactionId: "42", junk: "~~~>>>???øøø" };
  const jws = fakeJWS(payload);

  const encodedSegment = jws.split(".")[1];
  if (!encodedSegment.includes("-") && !encodedSegment.includes("_")) {
    throw new Error("test payload no longer exercises base64url characters");
  }

  assertEquals(decodeJWSPayload(jws), payload as unknown as Record<string, unknown>);
  assertEquals(decodeUnverifiedTransactionId(jws), "42");
});

Deno.test("handles stripped padding at every payload length", () => {
  for (let i = 0; i < 8; i++) {
    const payload = { transactionId: "9".repeat(i + 1) };
    assertEquals(decodeUnverifiedTransactionId(fakeJWS(payload)), payload.transactionId);
  }
});

Deno.test("numeric transactionId is stringified", () => {
  assertEquals(decodeUnverifiedTransactionId(fakeJWS({ transactionId: 12345 })), "12345");
});

Deno.test("returns null for garbage input", () => {
  assertEquals(decodeUnverifiedTransactionId("not-a-jws"), null);
  assertEquals(decodeUnverifiedTransactionId(""), null);
  assertEquals(decodeUnverifiedTransactionId("a.%%%%.c"), null);
});

Deno.test("returns null when transactionId is missing or wrong type", () => {
  assertEquals(decodeUnverifiedTransactionId(fakeJWS({ productId: "x" })), null);
  assertEquals(decodeUnverifiedTransactionId(fakeJWS({ transactionId: { nested: true } })), null);
});
