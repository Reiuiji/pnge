char * md5buffer(char *buffer, int length)
{
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;
	MD5_Init(&ctx);

	/* Process through the buffer */
	MD5_Update(&ctx, buffer, length);

	MD5_Final(digest, &ctx);

	/* Convert Digest to a string */
	char* md5sum = (char*) malloc (sizeof(char)*32);

	for (int i=0; i< 16; i++)
		sprintf(&md5sum[i*2], "%02x", (unsigned int)digest[i]);

	return md5sum;
}

