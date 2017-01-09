int wpi_encrypt(unsigned char * pofbiv_in,
				unsigned char * pbw_in,
				unsigned int plbw_in,
				unsigned char * pkey,
				unsigned char * pcw_out);

int wpi_decrypt(unsigned char * pofbiv_in,
				unsigned char * pcw_in,
				unsigned int plcw_in,
				unsigned char * prkey_in,
				unsigned char * pbw_out);

int wpi_pmac(unsigned char * pmaciv_in,
				unsigned char * pmac_in,
				unsigned int pmacpc_in,
				unsigned char * pkey,
				unsigned char * pmac_out);

