'use client';

import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { useSearchParamsObject } from '../utils/hooks';
import { searchParamPageSchema } from '../utils/page';
import { UrlObject } from 'url';
import { twMerge } from 'tailwind-merge';

function Button({
  label,
  disabled,
  href,
}: {
  label: string;
  disabled?: boolean;
  href: UrlObject;
}) {
  if (disabled) {
    return (
      <span className={'bg-[#f7f7f7] px-4 py-1.5 text-[#ccc]'}>{label}</span>
    );
  } else {
    return (
      <Link
        href={href}
        className={'bg-[#f7f7f7] px-4 py-1.5 text-[#970] hover:underline'}
      >
        {label}
      </Link>
    );
  }
}

export function Pagination({ pageCount }: { pageCount: number }) {
  const searchParams = useSearchParamsObject();
  const { page: currentPage } = searchParamPageSchema.parse(searchParams);
  const pathname = usePathname();

  const pages = [
    currentPage - 3,
    currentPage - 2,
    currentPage - 1,
    currentPage,
    currentPage + 1,
    currentPage + 2,
    currentPage + 3,
  ].filter((page) => page > 0 && page <= pageCount);

  if (pages[0] !== 1) {
    pages.unshift(1);
  }
  if (pages[pages.length - 1] !== pageCount && pageCount > 1) {
    pages.push(pageCount);
  }

  // Add "..." when there is a gap between pages
  for (let i = 1; i < pages.length; i++) {
    if (pages[i] - pages[i - 1] > 1) {
      pages.splice(i, 0, -1);
      i++;
    }
  }

  return (
    <div className="flex flex-row justify-between border rounded-md overflow-hidden text-sm divide-x">
      <Button
        label="Previous"
        href={{
          pathname,
          query: currentPage === 2 ? {} : { page: currentPage - 1 },
        }}
        disabled={currentPage === 1}
      />
      <div className="flex flex-row grow justify-center items-center divide-x">
        {pages.map((page) => {
          if (page === -1) {
            return (
              <span key={page} className="px-2">
                ...
              </span>
            );
          } else {
            return (
              <span key={page}>
                <Link
                  href={{
                    pathname,
                    query:
                      page === 1
                        ? {}
                        : {
                            page,
                          },
                  }}
                  className={twMerge(
                    'px-2',
                    page === currentPage
                      ? 'text-[#666] font-bold'
                      : 'text-[#970] hover:underline'
                  )}
                >
                  {page}
                </Link>
              </span>
            );
          }
        })}
      </div>
      <Button
        label="Next"
        href={{
          pathname,
          query: {
            page: currentPage + 1,
          },
        }}
        disabled={currentPage === pageCount}
      />
    </div>
  );
}
