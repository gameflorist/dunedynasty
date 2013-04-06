#ifndef MULTICHAR_H
#define MULTICHAR_H

#define FOURCC(A,B,C,D) ((A << 24) | (B << 16) | (C << 8) | D)

enum {
	CC_BLDG = FOURCC('B','L','D','G'),
	CC_CAT  = FOURCC('C','A','T',' '),
	CC_DATA = FOURCC('D','A','T','A'),
	CC_EVNT = FOURCC('E','V','N','T'),
	CC_FORM = FOURCC('F','O','R','M'),
	CC_INFO = FOURCC('I','N','F','O'),
	CC_MAP  = FOURCC('M','A','P',' '),
	CC_NAME = FOURCC('N','A','M','E'),
	CC_ORDR = FOURCC('O','R','D','R'),
	CC_PLYR = FOURCC('P','L','Y','R'),
	CC_RPAL = FOURCC('R','P','A','L'),
	CC_RTBL = FOURCC('R','T','B','L'),
	CC_SCEN = FOURCC('S','C','E','N'),
	CC_SINF = FOURCC('S','I','N','F'),
	CC_SSET = FOURCC('S','S','E','T'),
	CC_TEAM = FOURCC('T','E','A','M'),
	CC_TEXT = FOURCC('T','E','X','T'),
	CC_UNIT = FOURCC('U','N','I','T'),
	CC_XMID = FOURCC('X','M','I','D'),

	/* OpenDUNE extensions. */
	CC_ODUN = FOURCC('O','D','U','N'), /* OpenDUNE Unit New. */

	/* Dune Dynasty extensions. */
	CC_DDAI = FOURCC('D','D','A','I'), /* Dune Dynasty Brutal AI. */
	CC_DDB2 = FOURCC('D','D','B','2'), /* Dune Dynasty Building 2. */
	CC_DDI2 = FOURCC('D','D','I','2'), /* Dune Dynasty Info 2 (multiple selection). */
	CC_DDM2 = FOURCC('D','D','M','2'), /* Dune Dynasty Map 2 (fog of war). */
	CC_DDS2 = FOURCC('D','D','S','2'), /* Dune Dynasty Scenario 2 (skirmish alliances). */
	CC_DDU2 = FOURCC('D','D','U','2'), /* Dune Dynasty Unit 2. */
};

#undef FOURCC

#endif
