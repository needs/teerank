import Link from 'next/link';
import { twMerge } from 'tailwind-merge';

function Tab({
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

export default async function Index() {
  return (
    <div className="flex flex-col gap-4 py-8">
      <header className="flex flex-row bg-gradient-to-b from-transparent to-[#eaeaea] px-12">
        <Tab label="Players" count={590000} isActive={true} />
        <Tab label="Maps" count={60000} isActive={false} />
        <Tab label="Servers" count={1200} isActive={false} />
      </header>

      <main className="grid grid-cols-[auto_1fr_1fr_auto_auto] px-16 gap-x-8 gap-y-2">
        <span className="text-[#970] text-xl font-bold py-1"></span>
        <span className="text-[#970] text-xl font-bold py-1">Name</span>
        <span className="text-[#970] text-xl font-bold py-1">Clan</span>
        <span className="text-[#970] text-xl font-bold py-1">Elo</span>
        <span className="text-[#970] text-xl font-bold py-1">Last Seen</span>

        <span className="col-span-5 border-b"></span>

        <span>1</span>
        <span>
          <Link href="/" className="hover:underline">
            Player 1
          </Link>
        </span>
        <span>
          <Link href="/" className="hover:underline">
            POLICE
          </Link>
        </span>
        <span>1503</span>
        <span>1 hour ago</span>

        <span>2</span>
        <span>
          <Link href="/" className="hover:underline">
            Player 2
          </Link>
        </span>
        <span>
          <Link href="/" className="hover:underline">
            EVIL
          </Link>
        </span>
        <span>1503</span>
        <span>1 hour ago</span>
      </main>
    </div>
  );
}
