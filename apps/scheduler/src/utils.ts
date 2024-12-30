import { wait } from "@teerank/teerank";
import { captureException } from "@sentry/node";

export function schedule(every: number, callback: () => Promise<void>) {
  const callbackWithSentry = () => {
    callback().catch(error => {
      console.error(error);
      captureException(error);
    });
  }

  callbackWithSentry();
  setInterval(callbackWithSentry, every);
}

export function scheduleWithSpread(every: number, callback: () => Promise<void>) {
  const delay = Math.random() * every;
  wait(delay).then(() => schedule(every, callback));
}
