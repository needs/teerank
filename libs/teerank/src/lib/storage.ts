import { S3Client } from "@aws-sdk/client-s3";
import { getEnv } from "./utils";

export const S3_BUCKET = getEnv('S3_BUCKET', 'teerank-snapshots');

let s3Client: S3Client | null = null;

export function getS3Client() {
  s3Client ??= new S3Client({
    endpoint: getEnv('S3_ENDPOINT', 'http://localhost:9000'),
    region: getEnv('S3_REGION', 'auto'),
    forcePathStyle: getEnv('S3_FORCE_PATH_STYLE', 'true') === 'true',
    credentials: {
      accessKeyId: getEnv('S3_ACCESS_KEY_ID', 'minioadmin'),
      secretAccessKey: getEnv('S3_SECRET_ACCESS_KEY', 'minioadmin'),
    },
  });

  return s3Client;
}
