import { twMerge } from 'tailwind-merge';
import { isStubName } from '@teerank/teerank';

export function SharedRatio({
  pollCount,
  occurrenceCount,
  className,
}: {
  pollCount: number;
  occurrenceCount: number;
  className?: string;
}) {
  if (!isStubName(pollCount, occurrenceCount)) {
    return null;
  }

  const ratio = occurrenceCount / pollCount;

  return (
    <span
      title={`On ${ratio.toFixed(
        1
      )} servers at once on average over ${pollCount} polls: this name is used by many different people.`}
      className={twMerge(
        'relative inline-block cursor-help px-1.5 text-sm text-[#999]',
        className
      )}
    >
      <span className="absolute left-0 top-0 h-2 w-2 rounded-tl border-l border-t border-[#999]" />
      <span className="absolute bottom-0 right-0 h-2 w-2 rounded-br border-b border-r border-[#999]" />
      {ratio.toFixed(1)}
    </span>
  );
}
