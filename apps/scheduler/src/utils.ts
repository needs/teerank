import { wait } from "@teerank/teerank";

export function schedule(every: number, callback: () => Promise<void>) {
  callback();
  setInterval(callback, every);
}

export function scheduleWithSpread(every: number, callback: () => Promise<void>) {
  const delay = Math.random() * every;
  wait(delay).then(() => schedule(every, callback));
}
