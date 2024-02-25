import { MetadataRoute } from 'next'

export default function sitemap(): MetadataRoute.Sitemap {
  return [
    {
      url: 'https://teerank.io',
      lastModified: new Date(),
    },
    {
      url: 'https://teerank.io/clans',
      lastModified: new Date(),
    },
    {
      url: 'https://teerank.io/servers',
      lastModified: new Date(),
    },
    {
      url: 'https://teerank.io/about',
      lastModified: new Date(),
    },
    {
      url: 'https://teerank.io/gametypes',
      lastModified: new Date(),
    },
    {
      url: 'https://teerank.io/status',
      lastModified: new Date(),
    },
  ]
}
