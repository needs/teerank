import { formatDistanceToNow, intervalToDuration } from 'date-fns';
import Link from 'next/link';

export function LastSeen({
  lastSeen,
  currentServer,
}: {
  lastSeen: Date;
  currentServer: { ip: string; port: number } | null;
}) {
  if (currentServer !== null) {
    return (
      <Link
        href={{
          pathname: `/server/${currentServer.ip}/${currentServer.port}`,
        }}
        className="text-[#43a700] font-bold hover:underline"
      >
        Online
      </Link>
    );
  } else {
    const { years, months, weeks, days, hours } = intervalToDuration({
      start: lastSeen,
      end: new Date(),
    });

    let className;

    if ((years ?? 0) > 0 || (months ?? 0) > 0) {
      className = 'text-[#c6c6c6]';
    } else if ((weeks ?? 0) > 0 || (days ?? 0) > 0) {
      className = 'text-[#a7a7a7]';
    } else if ((hours ?? 0) > 0) {
      className = 'text-[#7fa764]';
    } else {
      className = 'text-[#56a721]';
    }

    return (
      <span className={className}>
        {formatDistanceToNow(new Date(lastSeen), {
          addSuffix: true,
        })}
      </span>
    );
  }
}
