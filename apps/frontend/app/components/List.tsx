import Link from 'next/link';

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
  const templateColumns = columns
    .map((column) => (column.expand ? '1fr' : 'auto'))
    .join('_');

  return (
    <main
      className={`grid grid-cols-[${templateColumns}] px-16 gap-x-8 gap-y-2`}
    >
      {columns.map((column) => (
        <span key={column.title} className="text-[#970] text-xl font-bold py-1">
          {column.title}
        </span>
      ))}

      <span className="col-span-5 border-b"></span>

      {children}
    </main>
  );
}

export function ListCell({ label, href, alignRight }: { label: string; href?: string, alignRight?: boolean }) {
  return (
    <span className={alignRight ? "text-right" : ""}>{href !== undefined ? <Link href={href} className='hover:underline'>{label}</Link> : label}</span>
  );
}
