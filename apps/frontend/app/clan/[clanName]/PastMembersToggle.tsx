"use client";

import { useRouter, useSearchParams } from "next/navigation";

export function PastMembersToggle({ clanName, showPastMembers }: { clanName: string, showPastMembers: boolean }) {
  const router = useRouter();
  const searchParams = useSearchParams();

  return (
    <div className="flex justify-end px-8 mb-2">
      <label className="flex items-center gap-2 text-sm text-[#999999] cursor-pointer">
        <input 
          type="checkbox" 
          checked={showPastMembers} 
          onChange={(e) => {
            const params = new URLSearchParams(searchParams.toString());
            if (e.target.checked) {
              params.set('past', 'true');
            } else {
              params.delete('past');
            }
            const queryString = params.toString();
            const url = `/clan/${encodeURIComponent(clanName)}${queryString ? `?${queryString}` : ''}`;
            router.replace(url, { scroll: false });
          }}
          className="cursor-pointer"
        />
        <span className="select-none">Show past members</span>
      </label>
    </div>
  );
}
