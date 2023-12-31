import Image from 'next/image';

export async function SearchForm() {
  return (
    <form
      className="flex flex-row rounded-md border divide-x overflow-hidden self-center border-[#ccc] text-md"
      action={`/search`}
    >
      <input
        name="query"
        type="text"
        className="px-4 bg-white focus:outline-none"
        placeholder="Search"
      />
      <button className="bg-gradient-to-b from-[#fbea63] to-[#ffa525]">
        <Image src="/search.svg" width={42} height={42} alt="Search" />
      </button>
    </form>
  );
}
