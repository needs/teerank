import Link from 'next/link';

export default function Index() {
  return (
    <main className="py-12 px-4 md:px-12 xl:px-20 text-[#666] flex flex-col gap-4">
      <h1 className="text-2xl font-bold clear-both">About</h1>
      <p>
        Teerank is an <i>unofficial</i> ranking system for{' '}
        <Link
          href="https://www.teeworlds.com/"
          className="text-[#970] hover:underline"
        >
          Teeworlds
        </Link>
        . It aims to be simple and fast to use. You can find the{' '}
        <Link
          href="https://github.com/needs/teerank"
          className="text-[#970] hover:underline"
        >
          source code on github
        </Link>
        . Teerank is a free software under{' '}
        <Link
          href="http://www.gnu.org/licenses/gpl-3.0.txt"
          className="text-[#970] hover:underline"
        >
          GNU GPL 3.0
        </Link>
        .
      </p>
      <p>
        Report any bugs or ideas on github issue tracker or send them by e-mail
        at{' '}
        <Link
          href="mailto:timothee.vialatoux@gmail.com"
          className="text-[#970] hover:underline"
        >
          timothee.vialatoux@gmail.com
        </Link>
        .
      </p>
      <h1 className="text-2xl font-bold clear-both">Under the hood</h1>

      <p>
        Teerank works by polling all servers every 5 minutes. For{' '}
        <Link
          href="https://en.wikipedia.org/wiki/Elo_rating_system"
          className="text-[#970] hover:underline"
        >
          Elo ranking
        </Link>
        , Elo are updated using the score difference between two polls. For Time
        ranking, the score is converted into a time. Play times are increased
        based on the time between two polls.
      </p>

      <p>
        {`The system as it is cannot know the team you are playing on, it also do
        not handle any sort of player authentification, thus faking is easy.
        That's why Teerank should not be taken too seriously, because it is
        built on a very naïve approach.`}
      </p>
    </main>
  );
}
