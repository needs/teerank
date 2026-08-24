import { getEnvFloat, getEnvInt } from "./utils";

export const STUB_OCCURRENCE_RATIO = getEnvFloat('STUB_OCCURRENCE_RATIO', 1.5);
export const STUB_MIN_POLL_COUNT = getEnvInt('STUB_MIN_POLL_COUNT', 10);

export function isStubName(pollCount: number, occurrenceCount: number) {
  return (
    pollCount >= STUB_MIN_POLL_COUNT &&
    occurrenceCount >= pollCount * STUB_OCCURRENCE_RATIO
  );
}
