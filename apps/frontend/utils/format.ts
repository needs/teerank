export function formatPlayTime(playTime: number) {
  const hours = Math.floor(playTime / (60 * 60));
  const minutes = Math.floor(playTime / 60) % 60;

  const hoursFormatted = Intl.NumberFormat('en-US', {
    maximumFractionDigits: 0,
  }).format(hours)

  const minutesFormatted = Intl.NumberFormat('en-US', {
    maximumFractionDigits: 0,
    minimumIntegerDigits: 2,
  }).format(minutes)

  return `${hoursFormatted}h${minutesFormatted}`;
}
