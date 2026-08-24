'use client';

import { usePathname, useRouter, useSearchParams } from 'next/navigation';
import { twMerge } from 'tailwind-merge';

function EyeIcon({ crossed }: { crossed: boolean }) {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      className="h-4 w-4"
    >
      <path d="M2.036 12.322a1.012 1.012 0 0 1 0-.639C3.423 7.51 7.36 4.5 12 4.5c4.638 0 8.573 3.007 9.963 7.178.07.207.07.431 0 .639C20.577 16.49 16.64 19.5 12 19.5c-4.638 0-8.573-3.007-9.963-7.178Z" />
      <circle cx="12" cy="12" r="3" />
      {crossed && <path d="M4 4l16 16" />}
    </svg>
  );
}

export function EyeToggle({
  label,
  title,
  param,
  value,
  visibleWhenSet,
  className,
}: {
  label: string;
  title: string;
  param: string;
  value: string;
  visibleWhenSet: boolean;
  className?: string;
}) {
  const router = useRouter();
  const pathname = usePathname();
  const searchParams = useSearchParams();
  const isSet = searchParams.get(param) === value;
  const visible = visibleWhenSet ? isSet : !isSet;

  const toggle = () => {
    const params = new URLSearchParams(searchParams.toString());

    if (isSet) {
      params.delete(param);
    } else {
      params.set(param, value);
    }

    const queryString = params.toString();
    router.replace(queryString ? `${pathname}?${queryString}` : pathname, {
      scroll: false,
    });
  };

  return (
    <button
      type="button"
      onClick={toggle}
      title={`${visible ? 'Hide' : 'Show'} ${title}`}
      className={twMerge(
        'inline-flex flex-row items-center gap-1.5 rounded border border-[#970] px-2 align-middle text-sm font-normal text-[#970] hover:bg-[#970] hover:text-white',
        className
      )}
    >
      <EyeIcon crossed={!visible} />
      {label}
    </button>
  );
}
