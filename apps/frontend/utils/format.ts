import { Duration, intervalToDuration } from "date-fns";

export function formatPlayTime(playTime: number) {
  const duration = intervalToDuration({
    start: 0,
    end: playTime * 1000,
  })

  const format = (amount1: number, unit1: string, amount2: number, unit2: string) => {
    const amount1Formatted = Intl.NumberFormat('en-US', {
      maximumFractionDigits: 0,
    }).format(amount1);

    const amount2Formatted = Intl.NumberFormat('en-US', {
      maximumFractionDigits: 0,
      minimumIntegerDigits: 2,
    }).format(amount2);

    return `${amount1Formatted}${unit1}${amount2Formatted}${unit2}`;
  }

  if ((duration.years ?? 0) > 0) {
    return format(duration.years ?? 0, 'y', duration.months ?? 0, 'm');
  } else if ((duration.months ?? 0) > 0) {
    return format(duration.months ?? 0, 'm', duration.days ?? 0, 'd');
  } else if ((duration.days ?? 0) > 0) {
    return format(duration.days ?? 0, 'd', duration.hours ?? 0, 'h');
  } else if ((duration.hours ?? 0) > 0) {
    return format(duration.hours ?? 0, 'h', duration.minutes ?? 0, 'm');
  } else if ((duration.minutes ?? 0) > 0) {
    return format(duration.minutes ?? 0, 'm', duration.seconds ?? 0, 's');
  } else {
    const secondsFormatted = Intl.NumberFormat('en-US', {
      maximumFractionDigits: 0,
    }).format(duration.seconds ?? 0);

    return `${secondsFormatted}s`;
  }
}

export function formatDurationShort(duration: Duration) {
  const format = (quantity: number, singular: string, plural: string) => {
    const quantityFormatted = Intl.NumberFormat('en-US', {
      maximumFractionDigits: 0,
    }).format(quantity);

    if (quantity === 1) {
      return `${quantityFormatted} ${singular}`;
    } else {
      return `${quantityFormatted} ${plural}`;
    }
  }

  if ((duration.years ?? 0) > 0) {
    return format(duration.years ?? 0, 'year', 'years');
  } else if ((duration.months ?? 0) > 0) {
    return format(duration.months ?? 0, 'month', 'months');
  } else if ((duration.days ?? 0) > 0) {
    return format(duration.days ?? 0, 'day', 'days');
  } else if ((duration.hours ?? 0) > 0) {
    return format(duration.hours ?? 0, 'hour', 'hours');
  } else if ((duration.minutes ?? 0) > 0) {
    return format(duration.minutes ?? 0, 'minute', 'minutes');
  } else {
    return format(duration.seconds ?? 0, 'second', 'seconds');
  }
}

export function formatInteger(number: number) {
  return Intl.NumberFormat('en-US', {
    maximumFractionDigits: 0,
  }).format(number);
}

export function formatCamelCase(text: string) {
  return text.replace(/([A-Z])/g, ' $1').replace(/^./, (str) => str.toUpperCase());
}

export function capitalize(text: string) {
  return text.charAt(0).toUpperCase() + text.slice(1).toLowerCase();
}
