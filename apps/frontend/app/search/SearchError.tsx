"use client";

export function Error({ message }: { message: string }) {
  return (
    <div className="flex flex-col items-stretch justify-center p-8">
      <span className="p-4 text-md bg-red-50 border border-red-200 text-red-400 rounded-md">
        {message}
      </span>
    </div>
  );
}
