#ifndef FCL_STRING_H
#define FCL_STRING_H

/**
 * Duplicate a string. This needs to be done since strdup is not ansi.
 *
 * @param[in] s String which shall be duplicated.
 * @return
 */
char *dupstr(const char *s);

#endif /* FCL_STRING_H */
