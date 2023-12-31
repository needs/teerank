import { useSearchParams } from "next/navigation";

export function useSearchParamsObject() {
  const searchParams = useSearchParams();
  return Object.fromEntries(searchParams.entries());
}
