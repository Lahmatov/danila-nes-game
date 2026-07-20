import { decodeJWSPayload } from "./jws.ts";

// Local deep-equal assert instead of jsr:@std/assert so tests run with no network access
function assertEquals(actual: unknown, expected: unknown) {
  const a = JSON.stringify(actual);
  const b = JSON.stringify(expected);
  if (a !== b) throw new Error(`assertEquals failed:\n  actual:   ${a}\n  expected: ${b}`);
}

function fakeSignedTransactionInfo(overrides: Record<string, unknown> = {}): string {
  const payload = {
    productId: "com.lahmatov.fairytalebook.subscription.monthly",
    expiresDate: Math.floor(Date.now() / 1000) + 86400 * 30,
    revocationDate: null,
    ...overrides,
  };

  function base64url(value: unknown): string {
    return btoa(JSON.stringify(value))
      .replace(/\+/g, "-")
      .replace(/\//g, "_")
      .replace(/=+$/, "");
  }

  return `${base64url({ alg: "ES256" })}.${base64url(payload)}.fake-signature`;
}

Deno.test("decodeJWSPayload extracts productId from signed transaction", () => {
  const signed = fakeSignedTransactionInfo();
  const payload = decodeJWSPayload(signed);

  assertEquals(payload.productId, "com.lahmatov.fairytalebook.subscription.monthly");
});

Deno.test("decodeJWSPayload handles expiresDate and revocationDate", () => {
  const expiresDate = 1700000000;
  const revocationDate = 1700001000;
  const signed = fakeSignedTransactionInfo({ expiresDate, revocationDate });
  const payload = decodeJWSPayload(signed);

  assertEquals(payload.expiresDate, expiresDate);
  assertEquals(payload.revocationDate, revocationDate);
});

Deno.test("decodeJWSPayload handles null revocationDate (active subscription)", () => {
  const signed = fakeSignedTransactionInfo({ revocationDate: null });
  const payload = decodeJWSPayload(signed);

  assertEquals(payload.revocationDate, null);
});

Deno.test("decodeJWSPayload handles missing expiresDate (non-subscription product)", () => {
  const signed = fakeSignedTransactionInfo({ expiresDate: undefined });
  const payload = decodeJWSPayload(signed);

  assertEquals(payload.expiresDate, undefined);
});

Deno.test("decodeJWSPayload handles single story purchase", () => {
  const signed = fakeSignedTransactionInfo({
    productId: "com.lahmatov.fairytalebook.story.single",
  });
  const payload = decodeJWSPayload(signed);

  assertEquals(payload.productId, "com.lahmatov.fairytalebook.story.single");
});
