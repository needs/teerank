-- CreateTable
CREATE TABLE `MasterServer` (
    `id` INTEGER NOT NULL AUTO_INCREMENT,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updatedAt` DATETIME(3) NOT NULL,
    `address` VARCHAR(191) NOT NULL,
    `port` INTEGER NOT NULL,

    UNIQUE INDEX `MasterServer_address_port_key`(`address`, `port`),
    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- CreateTable
CREATE TABLE `GameServer` (
    `id` INTEGER NOT NULL AUTO_INCREMENT,
    `createdAt` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
    `updatedAt` DATETIME(3) NOT NULL,
    `ip` VARCHAR(191) NOT NULL,
    `port` INTEGER NOT NULL,
    `masterServerId` INTEGER NOT NULL,

    UNIQUE INDEX `GameServer_ip_port_key`(`ip`, `port`),
    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
