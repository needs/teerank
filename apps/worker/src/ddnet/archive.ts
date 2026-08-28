import { GetObjectCommand, HeadObjectCommand, PutObjectCommand } from "@aws-sdk/client-s3";
import { S3_BUCKET, getS3Client } from "@teerank/teerank";

export function raceArchiveKey(ym: string, part: string) {
  return `ddnet/race/ym=${ym}/${part}.parquet`;
}

export function teamRaceArchiveKey(ym: string, part: string) {
  return `ddnet/teamrace/ym=${ym}/${part}.parquet`;
}

export async function putArchiveObject(key: string, body: Uint8Array) {
  const s3 = getS3Client();

  await s3.send(new PutObjectCommand({
    Bucket: S3_BUCKET,
    Key: key,
    Body: body,
    ContentType: 'application/vnd.apache.parquet',
  }));

  const head = await s3.send(new HeadObjectCommand({ Bucket: S3_BUCKET, Key: key }));

  if (head.ContentLength !== body.byteLength) {
    throw new Error(`Archive object ${key} size mismatch after upload`);
  }
}

export async function getArchiveObject(key: string) {
  const s3 = getS3Client();
  const object = await s3.send(new GetObjectCommand({ Bucket: S3_BUCKET, Key: key }));
  const body = await object.Body?.transformToByteArray();

  if (body === undefined) {
    throw new Error(`Archive object ${key} has no body`);
  }

  return body;
}
