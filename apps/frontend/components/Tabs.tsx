import Link from 'next/link';
import { twMerge } from 'tailwind-merge';
import { UrlObject } from 'url';
import { formatInteger } from '../utils/format';

export function Tabs({ children }: { children: React.ReactNode }) {
  return (
    <header className="flex flex-row bg-gradient-to-b from-transparent to-[#eaeaea] px-4 lg:px-8 xl:px-12">
      {children}
    </header>
  );
}

export function Tab({
  label,
  count,
  isActive,
  href,
}: {
  label: string;
  count: number;
  isActive: boolean;
  href: UrlObject;
}) {
  const className = twMerge(
    'flex flex-row justify-center p-2 text-base basis-full box-content',
    isActive
      ? 'bg-white rounded-t-md border border-[#dddddd] border-b-0'
      : 'border-b-2 border-b-transparent hover:border-b-[#999] text-[#999] hover:text-[#666] cursor-pointer'
  );

  const content = (
    <>
      <span className="text-xl font-bold px-4">{label}</span>
      <span className="text-[#999] text-lg px-4 hidden lg:block border-l">
        {formatInteger(count)}
      </span>
    </>
  );

  return isActive ? (
    <div className={className}>{content}</div>
  ) : (
    <Link href={href} className={className}>
      {content}
    </Link>
  );
}
