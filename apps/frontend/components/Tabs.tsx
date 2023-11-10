import { twMerge } from "tailwind-merge";

export function Tabs({ children }: { children: React.ReactNode }) {
  return (
    <header className="flex flex-row bg-gradient-to-b from-transparent to-[#eaeaea] px-12">
      {children}
    </header>
  );
}

export function Tab({
  label,
  count,
  isActive,
}: {
  label: string;
  count: number;
  isActive: boolean;
}) {
  return (
    <div
      className={twMerge(
        'flex flex-row justify-center p-2 text-base basis-full box-content',
        isActive
          ? 'bg-white rounded-t-md border border-[#dddddd] border-b-0'
          : 'border-b-2 border-b-transparent hover:border-b-[#999] text-[#999] hover:text-[#666] cursor-pointer'
      )}
    >
      <span className="text-xl font-bold border-r px-4">{label}</span>
      <span className="text-[#999] text-lg px-4">{count}</span>
    </div>
  );
}
