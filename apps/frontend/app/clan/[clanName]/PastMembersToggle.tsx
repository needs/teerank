"use client";

import { useRouter } from "next/navigation";

export function PastMembersToggle({ clanName, showPastMembers }: { clanName: string, showPastMembers: boolean }) {
  const router = useRouter();

  return (
    <div className="flex justify-end px-8 mb-2">
      <label className="flex items-center gap-2 text-sm text-[#999999] cursor-pointer">
        <input 
          type="checkbox" 
          checked={showPastMembers} 
          onChange={(e) => {
            const url = `/clan/${encodeURIComponent(clanName)}${e.target.checked ? '?past=true' : ''}`;
            router.replace(url, { scroll: false });
          }}
          className="cursor-pointer"
        />
        <span className="select-none">Show past members</span>
      </label>
    </div>
  );
}
