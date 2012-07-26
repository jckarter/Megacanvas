pid$1::*TileManager*require*:entry {
    self->timestamp[probefunc] = timestamp;
}

pid$1::*TileManager*require*:return
/self->timestamp[probefunc] != 0/
{
    printf("%d\n", timestamp - self->timestamp[probefunc]);
}
