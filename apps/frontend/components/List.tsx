import Link from 'next/link';
import { twMerge } from 'tailwind-merge';

type Column = {
  title: string;
  expand: boolean;
};

export function List({
  children,
  columns,
}: {
  children: React.ReactNode;
  columns: Column[];
}) {
  const gridTemplateColumns = columns
    .map((column) => (column.expand ? '1fr' : 'auto'))
    .join(' ');

  const gridColumn = `span ${columns.length} / span ${columns.length}`;

  return (
    <main
      className={`grid px-16 gap-x-8 gap-y-2`}
      style={{
        gridTemplateColumns,
      }}
    >
      {columns.map((column) => (
        <span key={column.title} className="text-[#970] text-xl font-bold py-1">
          {column.title}
        </span>
      ))}

      <span
        className="border-b"
        style={{
          gridColumn,
        }}
      />

      {children}
    </main>
  );
}

export function ListCell({
  label,
  href,
  alignRight,
}: {
  label: string;
  href?: string;
  alignRight?: boolean;
}) {
  return (
    <span
      className={twMerge('truncate', alignRight && 'text-right')}
    >
      {href !== undefined ? (
        <Link href={href} className="hover:underline">
          {label}
        </Link>
      ) : (
        label
      )}
    </span>
  );
}
