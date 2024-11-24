export async function wait(ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

export function randomRange(min: number, max: number) {
  return Math.random() * (max - min) + min;
}
