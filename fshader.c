#include <stdio.h>

int fshader (char *filename, int num, char **buffers) {
	
	if (filename == NULL || num <= 0 || buffers == NULL)
		return 0;
	
	FILE *file = fopen (filename, "r");
	if (file == NULL)
		return 0;
	
	fpos_t pos, start;
	char *version = "#version";
	int c, i = -1, j = 0, n = 0, w = 0, v = 0, end = 0;
	
	// Loop shader file
	while ((c = fgetc (file)) != EOF && i < num) {
	
		// Line feet
		if (c == '\n') {
			w += w ? 1 : 2; n++; 
			continue;
		}
		
		// White space
		if (c == ' ' || c == '\t') {
			if (!w) w = 1;
			continue;
		}
		
		// Comment
		if (c == '/') {
			fgetpos (file, &pos);
			c = fgetc (file);
			
			// Comment line
			if (c == '/') {
				while ((c = fgetc (file)) != EOF)
					if (c == '\n') {
						w += w ? 1 : 2; n++;
						break;
					}
				continue;
			}
			// Comment block
			else if (c == '*') {
				while ((c = fgetc (file)) != EOF) {
					if (c == '*' && fgetc (file) == '/') {
						if (!w) w = 1;
						break;
					} 
					else if (c == '\n') {
						w += w ? 1 : 2; n++;
					}
				}
				continue;
			}
			fsetpos (file, &pos);
			c = '/';
		}
		
		// Shader start
		{
			if (v >= 2)
				v = (!w && c == version[v]) ? v + 1 : 0;
			
			else if (v == 1)
				v = (w < 2 && c == 'v') ? 2 : 0;
			
			else if (v == 0 && c == '#') {
				fgetpos (file, &start);
				end = j;
				v = 1;
			}
			if (v == 8) {
				fsetpos (file, &start);
				if (++i > 0)
					buffers[i - 1][end] = '\0';
				if (i >= num)
					break;
				for (j = 0, end = n; end > 0; end--)
					buffers[i][j++] = '\n';
				c = '#';
				v = 0;
			}
		}

		// First shader
		if (i < 0) {
			w = 0;
			continue;
		}
				
		// Write buffer
		if (w) {
			if (w < 2) {
				buffers[i][j++] = ' ';
				w = 0;
			} 
			else for (w--; w > 0; w--)
				buffers[i][j++] = '\n';
		}
		buffers[i][j++] = c;
	}
	
	buffers[i][j] = '\0';
	
	return (i >= num) ? num : i + 1;
}