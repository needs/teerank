import { useSearchParams } from "next/navigation";
import { useEffect, useMemo, useState } from "react";
import { debounce } from "lodash";

export function useSearchParamsObject() {
  const searchParams = useSearchParams();
  return Object.fromEntries(searchParams.entries());
}

export function useDebounce<T>(value: T, wait: number): T {
  const [debounced, setDebounced] = useState(value);
  const setDebouncedSlowly = useMemo(() => debounce(setDebounced, wait), [wait]);

  useEffect(() => {
    setDebouncedSlowly(value);
  }, [value, setDebouncedSlowly]);

  useEffect(() => () => setDebouncedSlowly.cancel(), [setDebouncedSlowly]);

  return debounced;
}
