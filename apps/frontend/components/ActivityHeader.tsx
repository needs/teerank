import { ActivityPayload } from './ActivityCalendar';
import { ActivityCalendarSection } from './ActivityCalendarSection';

export function ActivityHeader({
  apiPath,
  activity,
  contentClassName,
  children,
}: {
  apiPath: string;
  activity: ActivityPayload;
  contentClassName: string;
  children: React.ReactNode;
}) {
  return (
    <div className="relative">
      <div className="hidden md:block">
        <ActivityCalendarSection apiPath={apiPath} initial={activity} />
      </div>

      <div className="flex max-w-full flex-row md:absolute md:inset-y-0 md:left-0 md:z-10">
        <div
          className={`flex min-w-0 bg-gradient-to-r from-white from-30% to-white/85 ${contentClassName}`}
        >
          {children}
        </div>
        <div className="hidden w-24 shrink-0 bg-gradient-to-r from-white/85 to-transparent md:block" />
      </div>
    </div>
  );
}
