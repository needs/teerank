-- CreateTable
CREATE TABLE `Player` (
    `name` VARCHAR(191) NOT NULL,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updatedAt` DATETIME(3) NOT NULL,

    PRIMARY KEY (`name`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- CreateTable
CREATE TABLE `GameServerClient` (
    `id` INTEGER NOT NULL AUTO_INCREMENT,
    `snapshotId` INTEGER NOT NULL,
    `name` VARCHAR(191) NOT NULL,
    `clan` VARCHAR(191) NOT NULL,
    `score` INTEGER NOT NULL,
    `country` VARCHAR(191) NOT NULL,
    `inGame` BOOLEAN NOT NULL,

    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- CreateTable
CREATE TABLE `GameServerSnapshot` (
    `id` INTEGER NOT NULL AUTO_INCREMENT,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `version` VARCHAR(191) NOT NULL,
    `name` VARCHAR(191) NOT NULL,
    `map` VARCHAR(191) NOT NULL,
    `gameType` VARCHAR(191) NOT NULL,
    `numPlayers` INTEGER NOT NULL,
    `maxPlayers` INTEGER NOT NULL,
    `numClients` INTEGER NOT NULL,
    `maxClients` INTEGER NOT NULL,
    `gameServerId` INTEGER NOT NULL,

    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
