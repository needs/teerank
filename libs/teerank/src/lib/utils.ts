export async function wait(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

export function randomRange(min: number, max: number) {
  return Math.random() * (max - min) + min;
}

export function getEnv(key: string, defaultValue: string) {
  const value = process.env[key];
  if (value === undefined) {
    return defaultValue;
  }
  return value;
}

export function getEnvInt(key: string, defaultValue: number) {
  const value = getEnv(key, defaultValue.toString());
  return Number(value);
}
