-- CreateTable
CREATE TABLE "MasterServer" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "address" TEXT NOT NULL,
    "port" INTEGER NOT NULL,

    CONSTRAINT "MasterServer_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "GameServer" (
    "id" SERIAL NOT NULL,
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,
    "ip" TEXT NOT NULL,
    "port" INTEGER NOT NULL,
    "masterServerId" INTEGER NOT NULL,

    CONSTRAINT "GameServer_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "MasterServer_address_port_key" ON "MasterServer"("address", "port");

-- CreateIndex
CREATE UNIQUE INDEX "GameServer_ip_port_key" ON "GameServer"("ip", "port");

-- AddForeignKey
ALTER TABLE "GameServer" ADD CONSTRAINT "GameServer_masterServerId_fkey" FOREIGN KEY ("masterServerId") REFERENCES "MasterServer"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
