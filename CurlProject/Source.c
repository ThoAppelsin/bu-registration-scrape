#define HAS_QUOTA_COLUMN

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#include "curl.h"

#define BASE_LINK						L"http://registration.boun.edu.tr"
#define SEMESTER_SELECTION_PAGE_LINK	BASE_LINK L"/schedule.htm"

const wchar_t BaseLink[ ]					= BASE_LINK;
const wchar_t SemesterSelectionPageLink[ ]	= SEMESTER_SELECTION_PAGE_LINK;

typedef struct ColumnIndexLookupTag
{
	size_t CodeSec;
	size_t Name;
	size_t Cr;
	size_t Instr;
	size_t Days;
	size_t Hours;
	size_t Rooms;
	size_t RawRequiredforDepts;
	size_t CodeSec2;
} ColumnIndexLookup;

typedef struct MyLookupTag
{
	wchar_t * Entry;
	size_t OrderedIndex;
} MyLookup;

typedef struct PhraseMatchInAmbiguityTag
{
	size_t NumberofWordsPassed;
	MyLookup * PLookupForMatch;
} PhraseMatchInAmbiguity;

typedef enum
{
	Monday = 1,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday,
	Sunday
} DAYS;

typedef struct ExtremesTag
{
	DAYS FirstDay;
	DAYS LastDay;
	unsigned int FirstSlot;
	unsigned int LastSlot;
	size_t LongestProgrammeEntryLength;
	size_t LongestLessonEntryLength;
	size_t LongestLessonNameEntryLength;
	size_t LongestCreditsEntryLength;
	size_t LongestLinkEntryLength;
	size_t LongestLectureTypeEntryLength;
	size_t LongestSectionEntryLength;
	size_t LongestInstructorEntryLength;
	size_t LongestRawDaysEntryLength;
	size_t LongestRawSlotsEntryLength;
	size_t LongestRawClassroomsEntryLength;
	size_t LongestClassroomEntryLength;
} Extremes;

typedef struct MyLectureTag
{
	DAYS Day;
	unsigned int Slot;
	MyLookup * PClassroomLookup;
} MyLecture;

typedef enum MySectionStateTag
{
	SectionState_Undefined,
	SectionState_TBA,
	SectionState_Failed,
	SectionState_OnlyDays,
	SectionState_DaysandSlots,
	SectionState_Complete
} MySectionState;

typedef struct MySectionTag
{
	MyLookup * PSectionLookup;
	MyLookup * PInstructorLookup;
	MyLookup * PRawDaysLookup;
	MyLookup * PRawSlotsLookup;
	MyLookup * PRawClassroomsLookup;
	MyLookup * PRawRequiredforDeptsLookup;
	MySectionState SectionState;
	size_t NumberofLectures;
	MyLecture * ALectures;
} MySection;

typedef struct MyBonusTag
{
	MyLookup * PBonusTypeLookup;
	MySection ** APSections;
} MyBonus;

typedef struct MyLessonTag
{
	MyLookup * PLessonLookup;
	MyLookup * PLessonNameLookup;
	MyLookup * PCreditsLookup;
	MyLookup * PLinkLookup;
	MySection ** APMainSections;
	MyBonus ** APBonuses;
} MyLesson;

typedef struct MyProgrammeTag
{
	MyLookup * PProgrammeLookup;
	MyLesson ** APLessons;
} MyProgramme;

MyLookup ** APProgrammeLookups = NULL;
MyLookup ** APLessonLookups = NULL;
MyLookup ** APLessonNameLookups = NULL;
MyLookup ** APCreditsLookups = NULL;
MyLookup ** APLinkLookups = NULL;
MyLookup ** APBonusTypeLookups = NULL;
MyLookup ** APSectionLookups = NULL;
MyLookup ** APInstructorLookups = NULL;
MyLookup ** APRawDaysLookups = NULL;
MyLookup ** APRawSlotsLookups = NULL;
MyLookup ** APRawClassroomsLookups = NULL;
MyLookup ** APRawRequiredforDeptsLookups = NULL;
MyLookup ** APClassroomLookups = NULL;

MyProgramme ** APProgrammes = NULL;

typedef struct
{
	size_t count;
	wchar_t * memory;
} MemoryStruct;

int WcsIsWcsWithStartAndEnd(const wchar_t * wcs1, const wchar_t * wcs2_start, const wchar_t * wcs2_end)
{
	while (*wcs1 && wcs2_start < wcs2_end && *wcs1 == *wcs2_start)
	{
		wcs1++;
		wcs2_start++;
	}
	return (*wcs1 == 0 && wcs2_start == wcs2_end) ? 1 : 0;
}

int WcsFoundAtWcsWithStartAndEnd(const wchar_t * wcs1, const wchar_t * wcs2_start, const wchar_t * wcs2_end)
{
	while (*wcs1 && wcs2_start < wcs2_end && *wcs1 == *wcs2_start)
	{
		wcs1++;
		wcs2_start++;
	}
	return (*wcs1 == 0) ? 1 : 0;
}

const wchar_t * wcswithstartandendstr(const wchar_t * start, const wchar_t * end, const wchar_t * const SubStr)
{
	const size_t SearchLength = (end - start) - wcslen(SubStr) + 1;
	for (size_t i = 0; i < SearchLength; i++)
	{
		for (size_t j = 0; start[j] == SubStr[j]; j++)
		{
			if (SubStr[j + 1] == L'\0')
			{
				return start;
			}
		}
		start++;
	}
	return NULL;
}

void wcscpywithstartandend(wchar_t * const Destinationwcs, const wchar_t * const Start, const wchar_t * const End)
{
	size_t Length = End - Start;
	wcsncpy(Destinationwcs, Start, Length);
	Destinationwcs[Length] = 0;
}

wchar_t * wcsdupwithstartandend(const wchar_t * const Start, const wchar_t * const End)
{
	wchar_t * TheDupe = malloc((End - Start + 1) * sizeof * TheDupe);
	if (TheDupe == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	wcscpywithstartandend(TheDupe, Start, End);
	return TheDupe;
}

const wchar_t * wcsstradvance(const wchar_t * const wcs, const wchar_t * const substr)
{
	const wchar_t * const result = wcsstr(wcs, substr);
	if (result)
		return result + wcslen(substr);
	else
		return result;
}

// LeftLimitwcs has to be within SourceStart
// and come before RightLimitwcs (also within SourceStart)
// else function will return NULL
const wchar_t * MatchFromRightToLeft(const wchar_t * SourceStart, const wchar_t * const LeftLimitwcs, const wchar_t * const RightLimitwcs,
									 const wchar_t ** const PMatchStart, const wchar_t ** const PMatchEnd)
{
	*PMatchEnd = wcsstr(SourceStart, RightLimitwcs);
	if (*PMatchEnd == NULL)
	{
		return NULL;
	}

	SourceStart = wcsstr(SourceStart, LeftLimitwcs);
	if (SourceStart == NULL)
	{
		return NULL;
	}
	while (SourceStart && SourceStart < *PMatchEnd)
	{
		*PMatchStart = SourceStart;
		SourceStart = wcsstr(SourceStart + 1, LeftLimitwcs);
	}
	*PMatchStart += wcslen(LeftLimitwcs);

	return *PMatchEnd + wcslen(RightLimitwcs);
}

const wchar_t * MatchFromLeftToRight(const wchar_t * const SourceStart, const wchar_t * const LeftLimitwcs, const wchar_t * const RightLimitwcs,
									 const wchar_t ** const PMatchStart, const wchar_t ** const PMatchEnd)
{
	*PMatchStart = wcsstr(SourceStart, LeftLimitwcs);
	if (*PMatchStart == NULL)
	{
		return NULL;
	}
	*PMatchStart += +wcslen(LeftLimitwcs);

	*PMatchEnd = wcsstr(*PMatchStart, RightLimitwcs);
	if (*PMatchEnd == NULL)
	{
		return NULL;
	}
	
	return *PMatchEnd + wcslen(RightLimitwcs);
}

void InitializeSection(MySection * const PSection)
{
	PSection->PSectionLookup = APSectionLookups[0];
	PSection->PInstructorLookup = APInstructorLookups[0];
	PSection->SectionState = SectionState_Undefined;
	PSection->PRawDaysLookup = APRawDaysLookups[0];
	PSection->PRawSlotsLookup = APRawSlotsLookups[0];
	PSection->PRawClassroomsLookup = APRawClassroomsLookups[0];
	PSection->PRawRequiredforDeptsLookup = APRawRequiredforDeptsLookups[0];
	PSection->NumberofLectures = 0U;
	PSection->ALectures = NULL;
}

MySection * OneMoreSection(MySection *** const PAPSections)
{
	size_t size = 0;
	while ((*PAPSections)[size] != NULL)
		size++;

	(*PAPSections) = realloc((*PAPSections), (size + 2) * sizeof * (*PAPSections));
	if ((*PAPSections) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPSections)[size] = malloc(sizeof * (*PAPSections)[size]);
	if ((*PAPSections)[size] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPSections)[size + 1] = NULL;

	InitializeSection((*PAPSections)[size]);

	return (*PAPSections)[size];
}

void InitializeLectureType(MyBonus * const PBonus)
{
	PBonus->PBonusTypeLookup = APBonusTypeLookups[0];

	PBonus->APSections = malloc(sizeof * PBonus->APSections);
	PBonus->APSections[0] = NULL;
}

void InitializeLesson(MyLesson * const PLesson)
{
	PLesson->PLessonLookup = APLessonLookups[0];
	PLesson->PLessonNameLookup = APLessonNameLookups[0];
	PLesson->PCreditsLookup = APCreditsLookups[0];
	PLesson->PLinkLookup = APLinkLookups[0];

	PLesson->APMainSections = malloc(sizeof * PLesson->APMainSections);
	PLesson->APMainSections[0] = NULL;

	PLesson->APBonuses = malloc(sizeof * PLesson->APBonuses);
	PLesson->APBonuses[0] = NULL;
}

void InitializeProgramme(MyProgramme * const PProgramme)
{
	PProgramme->PProgrammeLookup = APProgrammeLookups[0];
	
	PProgramme->APLessons = malloc(sizeof * PProgramme->APLessons);
	PProgramme->APLessons[0] = NULL;
}

void TrimSpacesAndNbsps(const wchar_t ** const start, const wchar_t ** const end)
{
	for (;;)
	{
		if ((*start) < (*end) && (*end)[-1] == L' ')
		{
			(*end)--;
		}
		else if ((*start) < (*end) - 5 && WcsIsWcsWithStartAndEnd(L"&nbsp;", (*end) - 6, (*end)))
		{
			(*end) -= 6;
		}
		else
		{
			break;
		}
	}

	while ((*start) < (*end) && (*start)[0] == L' ')
	{
		(*start)++;
	}
}

size_t AmountofSpacesinWcs(const wchar_t * wcs_start)
{
	while (*wcs_start)
		if (*wcs_start++ == L' ')
			return 1;

	return 0;
}

// finds the ID of TheEntry inside TheLookup
// creates a new entry inside TheLookup if necessary
// returns the ID
MyLookup * FindTheLookup(const wchar_t * const TheEntry_start, const wchar_t * const TheEntry_end, MyLookup *** const PAPLookup)
{
	size_t size = 0;
	for (; (*PAPLookup)[size] != NULL; size++)
		if (WcsIsWcsWithStartAndEnd((*PAPLookup)[size]->Entry, TheEntry_start, TheEntry_end))
			return (*PAPLookup)[size];

	(*PAPLookup) = realloc((*PAPLookup), (size + 2) * sizeof * (*PAPLookup));
	if ((*PAPLookup) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLookup)[size] = malloc(sizeof * (*PAPLookup)[size]);
	(*PAPLookup)[size]->Entry = wcsdupwithstartandend(TheEntry_start, TheEntry_end);
	(*PAPLookup)[size]->OrderedIndex = 0U;

	(*PAPLookup)[size + 1] = NULL;
	return (*PAPLookup)[size];
}

MyBonus * FindTheBonus(const wchar_t * const Start, const wchar_t * const End, MyBonus *** const PAPBonuses)
{
	size_t size = 0;
	MyLookup * PLectureTypeLookup = FindTheLookup(Start, End, &APBonusTypeLookups);

	for (; (*PAPBonuses)[size] != NULL; size++)
		if ((*PAPBonuses)[size]->PBonusTypeLookup == PLectureTypeLookup)
			return (*PAPBonuses)[size];

	(*PAPBonuses) = realloc((*PAPBonuses), (size + 2) * sizeof * (*PAPBonuses));
	if ((*PAPBonuses) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPBonuses)[size] = malloc(sizeof * (*PAPBonuses)[size]);
	if ((*PAPBonuses)[size] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPBonuses)[size] = malloc(sizeof * (*PAPBonuses)[size]);
	InitializeLectureType((*PAPBonuses)[size]);
	(*PAPBonuses)[size]->PBonusTypeLookup = PLectureTypeLookup;

	(*PAPBonuses)[size + 1] = NULL;
	return (*PAPBonuses)[size];
}

MyLesson * FindTheLesson(const wchar_t * const Start, const wchar_t * const End, MyLesson *** const PAPLessons)
{
	size_t size = 0;
	MyLookup * PLessonLookup = FindTheLookup(Start, End, &APLessonLookups);

	for (; (*PAPLessons)[size] != NULL; size++)
		if ((*PAPLessons)[size]->PLessonLookup == PLessonLookup)
			return (*PAPLessons)[size];

	(*PAPLessons) = realloc((*PAPLessons), (size + 2) * sizeof * (*PAPLessons));
	if ((*PAPLessons) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLessons)[size] = malloc(sizeof * (*PAPLessons)[size]);
	if ((*PAPLessons)[size] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLessons)[size] = malloc(sizeof * (*PAPLessons)[size]);
	InitializeLesson((*PAPLessons)[size]);
	(*PAPLessons)[size]->PLessonLookup = PLessonLookup;

	(*PAPLessons)[size + 1] = NULL;
	return (*PAPLessons)[size];
}

MyProgramme * FindTheProgramme(const wchar_t * const Start, const wchar_t * const End)
{
	size_t size = 0;
	MyLookup * PProgrammeLookup = FindTheLookup(Start, End, &APProgrammeLookups);

	for (; APProgrammes[size] != NULL; size++)
		if (APProgrammes[size]->PProgrammeLookup == PProgrammeLookup)
			return APProgrammes[size];

	APProgrammes = realloc(APProgrammes, (size + 2) * sizeof * APProgrammes);
	if (APProgrammes == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	APProgrammes[size] = malloc(sizeof * APProgrammes[size]);
	InitializeProgramme(APProgrammes[size]);
	APProgrammes[size]->PProgrammeLookup = PProgrammeLookup;

	APProgrammes[size + 1] = NULL;
	return APProgrammes[size];
}

PhraseMatchInAmbiguity FindMatchInAmbiguity(const wchar_t * start, size_t AmbiguousWordCount)
{
	PhraseMatchInAmbiguity result = {
		0, NULL
	};

	const wchar_t * end = start;
	for (; end[0] && AmbiguousWordCount; AmbiguousWordCount--)
	{
		end++;
		while (end[0] && end[0] != L' ')
		{
			end++;
		}
	}

	if (AmbiguousWordCount != 0)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	for (size_t NumberofWordsPassed = 0; NumberofWordsPassed < 3; NumberofWordsPassed++)
	{
		for (size_t IDoftheMatch = 0; APClassroomLookups[IDoftheMatch]; IDoftheMatch++)
		{
			if (WcsFoundAtWcsWithStartAndEnd(APClassroomLookups[IDoftheMatch]->Entry, start, end))
			{
				result.NumberofWordsPassed = NumberofWordsPassed;
				result.PLookupForMatch = APClassroomLookups[IDoftheMatch];
				return result;
			}
		}

		while (start[0] != L' ')
		{
			start++;
		}
		while (start[0] == L' ')
		{
			start++;
		}
	}

	return result;
}

void EvaluateRawClassrooms(MySection * const PSection)
{
	const wchar_t * start = PSection->PRawClassroomsLookup->Entry;

	size_t AtMost = 0U;
	size_t AtLeast = 0U;
	
	size_t NumberofCharacters = 0U;
	size_t NumberofDots = 0U;
	size_t NumberofNumbers = 0U;

	int Ambiguity = 0;

	size_t NumberofFreeCoupleables = 0U;

	for (const wchar_t * helper = start; helper[0];)
	{
		for (; helper[0] && helper[0] != L' '; helper++)
		{
			if (iswalpha(helper[0]))
				NumberofCharacters++;
			else if (iswdigit(helper[0]))
				NumberofNumbers++;
			else if (helper[0] == L'.')
				NumberofDots++;
		}

		/*
		Cdn CDN
		Cdn cdN
		Cdn C1dN1
		Cdn CDn
		Cdn Cdn
		Cdn
		CDn
		C2dN2

		if (rest)											cdn / cDn / cDN
		{													c & (n | D)
			Shouldn't exist according to observations.
		}
		else if (appendable)								Cdn
		{													C & d & n		// cdn is eliminated, C redundant
			NumberofFreeCoupleables++;						d & n
		}
		else
		{
			if (requirespreceeding)							CDN / cdN / C1dN1
			{												(C & D & N) | (c & d & N) | (C == 1 & d & N == 1)	// c tells everything for cdN
				if (NumberofFreeCoupleables)				(C & D & N) | c | (C == 1 & d & N == 1)				// re-ordering
				{											c | (C & D & N) | (C == 1 & d & N == 1)				// C redundant
					NumberofFreeCoupleables--;				c | (D & N) | (C == 1 & d & N == 1)					// d actually redundant, else it'd be a CDN and we already have it
					min++;
					max++;
				}
				else
				{
					Shouldn't be happening according to observations.
				}
			}
			else if (maybepreceeded)						CDn		// D tells them all
			{												D
				NumberofFreeCoupleables++;
			}
			else if (standalone)							C2dN2	// just else is enough
			{
				min++;
				max++;
			}

			if (There is at least one pair & There are even amount of free coupleables)
			{
				Minimum is ambiguous.
			}
			min += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
			max += NumberofFreeCoupleables;
			NumberofFreeCoupleables = 0;
		}
		*/

		if (!NumberofCharacters && (!NumberofNumbers || NumberofDots))
		{
			fwprintf(stderr, L"%d: encountered a nonconsidered pattern for a room name in (%ls)\n", __LINE__, PSection->PRawClassroomsLookup->Entry);
			return;
		}
		else if (!NumberofDots && !NumberofNumbers)
		{
			NumberofFreeCoupleables++;
		}
		else
		{
			if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && NumberofNumbers == 1))
			{
				if (NumberofFreeCoupleables)
				{
					NumberofFreeCoupleables--;
					AtLeast++;
					AtMost++;
				}
				else
				{
					fwprintf(stderr, L"%d: encountered a nonconsidered sequence for a room name in (%ls)\n", __LINE__, PSection->PRawClassroomsLookup->Entry);
					return;
				}
			}
			else if (NumberofDots)
			{
				NumberofFreeCoupleables++;
			}
			else
			{
				AtLeast++;
				AtMost++;
			}

			if (NumberofFreeCoupleables)
			{
				if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
				{
					Ambiguity = 1;
				}
				AtLeast += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
				AtMost += NumberofFreeCoupleables;
				NumberofFreeCoupleables = 0;
			}
		}

		NumberofCharacters = 0;
		NumberofDots = 0;
		NumberofNumbers = 0;

		while (helper[0] == L' ')
		{
			helper++;
		}
	}

	if (NumberofFreeCoupleables)
	{
		if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
		{
			Ambiguity = 1;
		}
		AtLeast += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
		AtMost += NumberofFreeCoupleables;
		NumberofFreeCoupleables = 0;
	}

	NumberofCharacters = 0;
	NumberofDots = 0;
	NumberofNumbers = 0;

	if (AtMost == PSection->NumberofLectures)
	{
		MyLecture * PLecture = PSection->ALectures;
		const wchar_t * helperess = NULL;

		for (const wchar_t * helper = start; helper[0];)
		{
			for (; helper[0] && helper[0] != L' '; helper++)
			{
				if (iswalpha(helper[0]))
					NumberofCharacters++;
				else if (iswdigit(helper[0]))
					NumberofNumbers++;
				else if (helper[0] == L'.')
					NumberofDots++;
			}

			// requires preceeding (will be preceded)
			// may be preceeded (won't be preceeded)
			// must stay alone (won't be preceeded)
			// may be appended (won't be preceeded)

			if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && !NumberofDots && NumberofNumbers == 1))
			// requires preceeding
			{
				PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
				PLecture++;
				start = helper;
				while (start[0] == L' ')
				{
					start++;
				}
				helperess = NULL;
			}
			else if (NumberofDots || NumberofNumbers)
			// may be preceded or must stand alone
			{
				if (helperess)
				{
					PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
					PLecture++;
					start = helperess;
					while (start[0] == L' ')
					{
						start++;
					}
					helperess = NULL;
				}
				PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
				PLecture++;
				start = helper;
				while (start[0] == L' ')
				{
					start++;
				}
			}
			else
			// may be appended
			{
				if (helperess)
				{
					PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
					PLecture++;
					start = helperess;
					while (start[0] == L' ')
					{
						start++;
					}
				}
				helperess = helper;
			}

			NumberofCharacters = 0;
			NumberofDots = 0;
			NumberofNumbers = 0;

			while (helper[0] == L' ')
			{
				helper++;
			}
		}

		if (helperess)
		{
			PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
			PLecture++;
			start = helperess;
			while (start[0] == L' ')
			{
				start++;
			}
		}
		PSection->SectionState = SectionState_Complete;
		return;
	}

	if (AtLeast == PSection->NumberofLectures)
	{
		if (!Ambiguity)
		{
			MyLecture * PLecture = PSection->ALectures;
			const wchar_t * helperess = NULL;
			const wchar_t * helpjester = NULL;

			for (const wchar_t * helper = start; helper[0];)
			{
				for (; helper[0] && helper[0] != L' '; helper++)
				{
					if (iswalpha(helper[0]))
						NumberofCharacters++;
					else if (iswdigit(helper[0]))
						NumberofNumbers++;
					else if (helper[0] == L'.')
						NumberofDots++;
				}

				// requires preceeding (will be preceded)
				// may be preceeded (will be preceeded, if available)
				// must stay alone (won't be preceeded)
				// may be appended (will be preceeded, if available and next one isn't 'requires preceeding')

				if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && !NumberofDots && NumberofNumbers == 1))
				// requires preceeding
				{
					if (helpjester)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helpjester, &APClassroomLookups);
						PLecture++;
						start = helpjester;
						while (start[0] == L' ')
						{
							start++;
						}
						helpjester = NULL;
					}
					PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
					PLecture++;
					start = helper;
					while (start[0] == L' ')
					{
						start++;
					}
					helperess = NULL;
				}
				else if (NumberofDots)
				// may be preceded
				{
					if (helperess)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
						while (start[0] == L' ')
						{
							start++;
						}
						helperess = NULL;
					}
					else
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
						while (start[0] == L' ')
						{
							start++;
						}
					}
				}
				else if (NumberofNumbers)
				// must stand alone
				{
					if (helpjester)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
						PLecture++;
						start = helperess;
						while (start[0] == L' ')
						{
							start++;
						}
						helpjester = NULL;
						helperess = NULL;
					}
					else if (helperess)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
						PLecture++;
						start = helperess;
						while (start[0] == L' ')
						{
							start++;
						}
						helperess = NULL;
					}

					PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
					PLecture++;
					start = helper;
					while (start[0] == L' ')
					{
						start++;
					}
				}
				else
				// may be appended
				{
					if (helpjester)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
						PLecture++;
						start = helperess;
						while (start[0] == L' ')
						{
							start++;
						}
						helpjester = NULL;
						helperess = helper;
					}
					else if (helperess)
					{
						helpjester = helperess;
						helperess = helper;
					}
					helperess = helper;
				}

				NumberofCharacters = 0;
				NumberofDots = 0;
				NumberofNumbers = 0;

				while (helper[0] == L' ')
				{
					helper++;
				}
			}

			if (helperess)
			{
				PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
				PLecture++;
				start = helperess;
				while (start[0] == L' ')
				{
					start++;
				}
			}

			PSection->SectionState = SectionState_Complete;
			return;
		}
		else
		{
			// it is ambiguous, but we shall make an educated guess
			// using the data about classrooms we have thus far
			Ambiguity = 0;
			NumberofFreeCoupleables = 0;
			MyLecture * PLecture = PSection->ALectures;
			const wchar_t * helper;
			
			for (helper = start; helper[0];)
			{
				for (; helper[0] && helper[0] != L' '; helper++)
				{
					if (iswalpha(helper[0]))
						NumberofCharacters++;
					else if (iswdigit(helper[0]))
						NumberofNumbers++;
					else if (helper[0] == L'.')
						NumberofDots++;
				}
				while (helper[0] && helper[1] == L' ')
					helper++;

				if (!NumberofDots && !NumberofNumbers)
				{
					NumberofFreeCoupleables++;
				}
				else
				{
					if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && NumberofNumbers == 1))
					{
						NumberofFreeCoupleables--;
						if (NumberofFreeCoupleables == 0)
						{
							PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
							PLecture++;
							start = helper;
							while (start[0] == L' ')
							{
								start++;
							}
						}
					}
					else if (NumberofDots)
					{
						NumberofFreeCoupleables++;
					}
					else
					{
						if (NumberofFreeCoupleables == 0)
						{
							PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
							PLecture++;
							start = helper;
							while (start[0] == L' ')
							{
								start++;
							}
						}
					}

					if (NumberofFreeCoupleables)
					{
						if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
						{
							// case is ambiguous, try to resolve
							PhraseMatchInAmbiguity result = FindMatchInAmbiguity(start, NumberofFreeCoupleables);
							if (result.PLookupForMatch == NULL)
							{
								// ambiguity couldn't be resolved
								return;
							}
							if (result.NumberofWordsPassed)
							{
								for (helper = start; result.NumberofWordsPassed; result.NumberofWordsPassed--)
								{
									while (helper[0] == L' ')
									{
										helper++;
									}
									while (helper[0] != L' ')
									{
										helper++;
									}
								}
								PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
								PLecture++;
								start = helper;
							}
							PLecture->PClassroomLookup = result.PLookupForMatch;
							PLecture++;
							wcsstradvance(start, result.PLookupForMatch->Entry);
							while (start[0] == L' ')
							{
								start++;
							}
						}
						else
						{
							if (NumberofFreeCoupleables == 1)
							{
								helper = start;
								while (helper[0] != L' ')
								{
									helper++;
								}
								PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
								PLecture++;
								start = helper;
								while (start[0] == L' ')
								{
									start++;
								}
							}
							else /*if (NumberofFreeCoupleables % 2 == 0)*/
							{
								for (size_t i = 0; i < NumberofFreeCoupleables / 2; i++)
								{
									helper = start;
									while (helper[0] != L' ')
									{
										helper++;
									}
									while (helper[0] == L' ')
									{
										helper++;
									}
									while (helper[0] != L' ')
									{
										helper++;
									}

									PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
									PLecture++;
									start = helper;
									while (start[0] == L' ')
									{
										start++;
									}
								}
							}
						}
						NumberofFreeCoupleables = 0;
					}
					helper = start;
				}

				NumberofCharacters = 0;
				NumberofDots = 0;
				NumberofNumbers = 0;
			}

			if (NumberofFreeCoupleables)
			{
				if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
				{
					// case is ambiguous, try to resolve
					PhraseMatchInAmbiguity result = FindMatchInAmbiguity(start, NumberofFreeCoupleables);
					if (result.PLookupForMatch == NULL)
					{
						// ambiguity couldn't be resolved
						return;
					}
					if (result.NumberofWordsPassed)
					{
						for (helper = start; result.NumberofWordsPassed; result.NumberofWordsPassed--)
						{
							while (helper[0] == L' ')
							{
								helper++;
							}
							while (helper[0] != L' ')
							{
								helper++;
							}
						}
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
					}
					PLecture->PClassroomLookup = result.PLookupForMatch;
					PLecture++;
					wcsstradvance(start, result.PLookupForMatch->Entry);
					while (start[0] == L' ')
					{
						start++;
					}
				}
				else
				{
					if (NumberofFreeCoupleables == 1)
					{
						helper = start;
						while (helper[0] != L' ')
						{
							helper++;
						}
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
						while (start[0] == L' ')
						{
							start++;
						}
					}
					else /*if (NumberofFreeCoupleables % 2 == 0)*/
					{
						for (size_t i = 0; i < NumberofFreeCoupleables / 2; i++)
						{
							helper = start;
							while (helper[0] != L' ')
							{
								helper++;
							}
							while (helper[0] == L' ')
							{
								helper++;
							}
							while (helper[0] != L' ')
							{
								helper++;
							}

							PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
							PLecture++;
							start = helper;
							while (start[0] == L' ')
							{
								start++;
							}
						}
					}
				}
				NumberofFreeCoupleables = 0;
			}
			PSection->SectionState = SectionState_Complete;
		}
	}
}

void EvaluateRawSlots(MySection * const PSection)
{
	const wchar_t * start = PSection->PRawSlotsLookup->Entry;
	const size_t RawSlotsLength = wcslen(start);

	size_t AtMost = RawSlotsLength;
	for (const wchar_t * helper = start; helper[0]; helper++)
	{
		if (!iswdigit(helper[0]))
		{
			// wild character
			return;
		}
		if (helper[0] == L'0')
		{
			if (helper > start && helper[-1] == L'1')
			{
				AtMost--;
			}
			else
			{
				// 0 not preceded by 1
				return;
			}
		}
	}
	if (AtMost == PSection->NumberofLectures)
	{
		for (size_t i = 0; i < AtMost; i++)
		{
			if (start[0] == L'1' && start[1] == L'0')
			{
				PSection->ALectures[i].Slot = 10;
				start += 2;
			}
			else
			{
				PSection->ALectures[i].Slot = start[0] - L'0';
				start++;
			}
		}
		PSection->SectionState = SectionState_DaysandSlots;
		return;
	}

	size_t AtLeast = 0;
	for (const wchar_t * helper = start; helper[0];)
	{
		size_t NumberofFreeCoupleables = 0U;
		while (helper[0] == L'1')
		{
			NumberofFreeCoupleables++;
			helper++;
		}
		if (helper[0] == L'0')
		{
			NumberofFreeCoupleables--;
			AtLeast++;
			helper++;
		}
		else if (helper[0])
		{
			NumberofFreeCoupleables++;
			helper++;
		}
		
		if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
		{
			// ambiguous
			return;
		}

		AtLeast += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
	}
	if (AtLeast == PSection->NumberofLectures)
	{
		for (size_t i = 0U; i < AtLeast; i++)
		{
			if (start[0] == L'1')
			{
				if (start[1] == L'1' && start[2] == L'0')
				{
					PSection->ALectures[i].Slot = 1U;
					i++;
					PSection->ALectures[i].Slot = 10U;
					start += 3;
				}
				else
				{
					PSection->ALectures[i].Slot = 10U + start[1] - L'0';
					start += 2;
				}
			}
			else
			{
				PSection->ALectures[i].Slot = start[0] - L'0';
				start++;
			}
		}
		PSection->SectionState = SectionState_DaysandSlots;
		return;
	}
}

void EvaluateRawDays(MySection * const PSection)
{
	for (const wchar_t * helper = PSection->PRawDaysLookup->Entry; *helper; helper++)
	{
		switch (*helper)
		{
			case L'M':
			case L'W':
			case L'F':
				PSection->NumberofLectures++;
				break;
			case L'T':
				if (helper[1] == L'h' || helper[1] == L'H')
				{
					helper++;
				}
				PSection->NumberofLectures++;
				break;
			case L'S':
				if (helper[1] == L't' || helper[1] == L'T')
				{
					helper++;
				}
				PSection->NumberofLectures++;
				break;
			default:
				PSection->SectionState = SectionState_Failed;
				return;
		}
	}

	// allocate array of lectures, initialize Slots and Classrooms with zeroes
	PSection->ALectures = realloc(PSection->ALectures, PSection->NumberofLectures * sizeof * PSection->ALectures);
	for (size_t i = 0; i < PSection->NumberofLectures; i++)
	{
		PSection->ALectures[i].Slot = 0U;
		PSection->ALectures[i].PClassroomLookup = APClassroomLookups[0];
	}

	MyLecture * PLecture = PSection->ALectures;

	for (const wchar_t * helper = PSection->PRawDaysLookup->Entry; *helper; helper++, PLecture++)
	{
		switch (*helper)
		{
			case L'M':
				PLecture->Day = Monday;
				break;
			case L'W':
				PLecture->Day = Wednesday;
				break;
			case L'F':
				PLecture->Day = Friday;
				break;
			case L'T':
				if (helper[1] == L'h' || helper[1] == L'H')
				{
					helper++;
					PLecture->Day = Thursday;
				}
				else
				{
					PLecture->Day = Tuesday;
				}
				break;
			case L'S':
				if (helper[1] == L't' || helper[1] == L'T')
				{
					helper++;
					PLecture->Day = Saturday;
				}
				else
				{
					PLecture->Day = Sunday;
				}
				break;
		}
	}

	PSection->SectionState = SectionState_OnlyDays;
}

const wchar_t * OrdinalSuffix(size_t Decimal)
{
	switch (Decimal % 10)
	{
		case 1:
			return L"st";
		case 2:
			return L"nd";
		case 3:
			return L"rd";
		default:
			return L"th";
	}
}

size_t FindCellIndexWithinRow(const wchar_t * RowStart, const wchar_t * RowEnd, const wchar_t * const Matchwcs, size_t NthOccurrence)
{
	if (NthOccurrence == 0)
	{
		return 0;
	}

	const wchar_t * MatchStart = wcsstr(RowStart, Matchwcs);
	if (MatchStart == NULL || MatchStart > RowEnd)
	{
		fwprintf(stderr, L"%d: no such string (%ls) within row\n", __LINE__, Matchwcs);
		return 0;
	}

	for (size_t i = 1; i != NthOccurrence; i++)
	{
		MatchStart = wcsstr(MatchStart + 1, Matchwcs);
		if (MatchStart == NULL || MatchStart > RowEnd)
		{
			fwprintf(stderr, L"%d: no %u%ls occurrence of string (%ls) within row\n", __LINE__, NthOccurrence, OrdinalSuffix(NthOccurrence), Matchwcs);
			return 0;
		}
	}

	// no need to be too excessive, we will assume
	// RowStart -- MatchStart -- RowEnd
	// and that row properly has cells within

	RowStart = wcsstr(RowStart, L"</td>");
	size_t CellIndex;
	for (CellIndex = 1; RowStart < MatchStart; CellIndex++)
	{
		RowStart = wcsstr(RowStart + 1, L"</td>");
	}

	return CellIndex;
}

void PrintLookupIntoFile(FILE * const LookupFile, const MyLookup * const * PFirstLookupP)
{
	fwprintf(LookupFile, L"%ls", (*PFirstLookupP)->Entry);
	for (PFirstLookupP++; *PFirstLookupP; PFirstLookupP++)
	{
		fwprintf(LookupFile, L"|%ls", (*PFirstLookupP)->Entry);
	}
}

void PrintProgrammesIntoFile(FILE * const ProgrammesFile)
{
	/*
	Programme (|):
		0...:
			0: ProgrammeLookupIndex
			a
			1: Lesson (b):
				0...:
					0: LessonLookupIndex
					c
					1: LessonNameLookupIndex
					c
					2: CreditsLookupIndex
					c
					3: LinkLookupIndex
					c
					4: Main Section (d):
						0...:
							0: SectionLookupIndex
							e
							1: InstructorLookupIndex
							e
							2: RawDaysLookupIndex
							e
							3: RawSlotsLookupIndex
							e
							4: RawClassroomsLookupIndex
							e
							5: RawRequiredforDeptsLookupIndex
							e
							6: SectionState
							e
							7: Lecture (f):
								0...:
									0: Day
									g
									1: Slot
									g
									2: ClassroomLookupIndex
						c
						5: Bonuses (d):
							0...:
								0: BonusTypeLookupIndex
								e
								1: Bonus Section (f):
									0...:
										0: SectionLookupIndex
										g
										1: InstructorLookupIndex
										g
										2: RawDaysLookupIndex
										g
										3: RawSlotsLookupIndex
										g
										4: RawClassroomsLookupIndex
										g
										5: RawRequiredforDeptsLookupIndex
										g
										6: SectionState
										g
										7: Lecture (h):
											0...:
												0: Day
												i
												1: Slot
												i
												2: ClassroomLookupIndex
	*/

	{
		int FirstinArray = 1;
		for (const MyProgramme * const * PFirstProgrammeP = APProgrammes; *PFirstProgrammeP; PFirstProgrammeP++, FirstinArray = 0)
		{
			fwprintf(ProgrammesFile, FirstinArray ? L"%ua" : L"|%ua",
					 (*PFirstProgrammeP)->PProgrammeLookup->OrderedIndex
					 );

			{
				int FirstinArray = 1;
				for (const MyLesson * const * PFirstLessonP = (*PFirstProgrammeP)->APLessons; *PFirstLessonP; PFirstLessonP++, FirstinArray = 0)
				{
					fwprintf(ProgrammesFile, FirstinArray ? L"%uc%uc%uc%uc" : L"b%uc%uc%uc%uc",
							 (*PFirstLessonP)->PLessonLookup->OrderedIndex,
							 (*PFirstLessonP)->PLessonNameLookup->OrderedIndex,
							 (*PFirstLessonP)->PCreditsLookup->OrderedIndex,
							 (*PFirstLessonP)->PLinkLookup->OrderedIndex
							 );

					{
						int FirstinArray = 1;
						for (const MySection * const * PFirstMainSectionP = (*PFirstLessonP)->APMainSections; *PFirstMainSectionP; PFirstMainSectionP++, FirstinArray = 0)
						{
							fwprintf(ProgrammesFile, FirstinArray ? L"%ue%ue%ue%ue%ue%ue%ue" : L"d%ue%ue%ue%ue%ue%ue%ue",
									 (*PFirstMainSectionP)->PSectionLookup->OrderedIndex,
									 (*PFirstMainSectionP)->PInstructorLookup->OrderedIndex,
									 (*PFirstMainSectionP)->PRawDaysLookup->OrderedIndex,
									 (*PFirstMainSectionP)->PRawSlotsLookup->OrderedIndex,
									 (*PFirstMainSectionP)->PRawClassroomsLookup->OrderedIndex,
									 (*PFirstMainSectionP)->PRawRequiredforDeptsLookup->OrderedIndex,
									 (unsigned int) (*PFirstMainSectionP)->SectionState
									 );

							for (size_t LectureIndex = 0; LectureIndex < (*PFirstMainSectionP)->NumberofLectures; LectureIndex++)
							{
								fwprintf(ProgrammesFile, (LectureIndex == 0) ? L"%ug%ug%u" : L"f%ug%ug%u",
										 (*PFirstMainSectionP)->ALectures[LectureIndex].Day,
										 (*PFirstMainSectionP)->ALectures[LectureIndex].Slot,
										 (*PFirstMainSectionP)->ALectures[LectureIndex].PClassroomLookup->OrderedIndex
										 );
							}
						}
					}

					fwprintf(ProgrammesFile, L"c");

					{
						int FirstinArray = 1;
						for (const MyBonus * const * PFirstBonusP = (*PFirstLessonP)->APBonuses; *PFirstBonusP; PFirstBonusP++, FirstinArray = 0)
						{
							fwprintf(ProgrammesFile, FirstinArray ? L"%ue" : L"d%ue",
									 (*PFirstBonusP)->PBonusTypeLookup->OrderedIndex
									 );

							{
								int FirstinArray = 1;
								for (const MySection * const * PFirstSectionP = (*PFirstBonusP)->APSections; *PFirstSectionP; PFirstSectionP++, FirstinArray = 0)
								{
									fwprintf(ProgrammesFile, FirstinArray ? L"%ug%ug%ug%ug%ug%ug%ug" : L"f%ug%ug%ug%ug%ug%ug%ug",
											 (*PFirstSectionP)->PSectionLookup->OrderedIndex,
											 (*PFirstSectionP)->PInstructorLookup->OrderedIndex,
											 (*PFirstSectionP)->PRawDaysLookup->OrderedIndex,
											 (*PFirstSectionP)->PRawSlotsLookup->OrderedIndex,
											 (*PFirstSectionP)->PRawClassroomsLookup->OrderedIndex,
											 (*PFirstSectionP)->PRawRequiredforDeptsLookup->OrderedIndex,
											 (unsigned int) (*PFirstSectionP)->SectionState
											 );

									for (size_t LectureIndex = 0; LectureIndex < (*PFirstSectionP)->NumberofLectures; LectureIndex++)
									{
										fwprintf(ProgrammesFile, (LectureIndex == 0) ? L"%ui%ui%u" : L"h%ui%ui%u",
												 (*PFirstSectionP)->ALectures[LectureIndex].Day,
												 (*PFirstSectionP)->ALectures[LectureIndex].Slot,
												 (*PFirstSectionP)->ALectures[LectureIndex].PClassroomLookup->OrderedIndex
												 );
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

int MatchCellContents(const wchar_t * RowStart, const wchar_t * RowEnd, const size_t CellIndex, const wchar_t ** const PCellStart, const wchar_t ** const PCellEnd)
{
	if (RowEnd == NULL)
	{
		fwprintf(stderr, L"%d: such row has no end\n", __LINE__);
		return 0;
	}

	RowStart = wcsstr(RowStart, L"<td");
	for (size_t i = 1; i < CellIndex; i++)
	{
		RowStart = wcsstr(RowStart + 1, L"<td");
		if (RowStart == NULL || RowStart >= RowEnd)
		{
			fwprintf(stderr, L"%d: there is no cell #%u within current row (%u)\n", __LINE__, CellIndex, i);
			return 0;
		}
	}

	if (MatchFromRightToLeft(RowStart, L">", L"</", PCellStart, PCellEnd) == NULL)
	{
		fwprintf(stderr, L"%d: cell contents could not be grabbed\n", __LINE__);
		return 0;
	}
	TrimSpacesAndNbsps(PCellStart, PCellEnd);

	return 1;
}

void InterpretDepartmentPage(const MemoryStruct * const data, MyLookup * const PLinkLookup)
{
	const wchar_t * RowStart	= data->memory;
	const wchar_t * RowEnd		= NULL;
	const wchar_t * CellStart	= NULL;
	const wchar_t * CellEnd		= NULL;
	const wchar_t * helper		= NULL;


	// Column Index Lookup creation
	RowStart = wcsstradvance(RowStart, L"<tr class=\"schtitle");
	if (RowStart == NULL)
	{
		fwprintf(stderr, L"%d: title row missing in %ls\n", __LINE__, PLinkLookup->Entry);
		return;
	}
	RowStart = wcsstradvance(RowStart, L">");
	if (RowStart == NULL)
	{
		fwprintf(stderr, L"%d: title row missing in %ls\n", __LINE__, PLinkLookup->Entry);
		return;
	}
	RowEnd = wcsstr(RowStart, L"</tr>");
	if (RowEnd == NULL)
	{
		fwprintf(stderr, L"%d: title row missed ending tag in %ls\n", __LINE__, PLinkLookup->Entry);
		return;
	}
	ColumnIndexLookup columnindexlookup;
	columnindexlookup.CodeSec	= FindCellIndexWithinRow(RowStart, RowEnd, L"Code.Sec", 1U);
	columnindexlookup.Name		= FindCellIndexWithinRow(RowStart, RowEnd, L"Name", 1U);
	columnindexlookup.Cr		= FindCellIndexWithinRow(RowStart, RowEnd, L"Cr.", 1U);
	columnindexlookup.Instr		= FindCellIndexWithinRow(RowStart, RowEnd, L"Instr.", 1U);
	columnindexlookup.Days		= FindCellIndexWithinRow(RowStart, RowEnd, L"Days", 1U);
	columnindexlookup.Hours		= FindCellIndexWithinRow(RowStart, RowEnd, L"Hours", 1U);
	columnindexlookup.Rooms		= FindCellIndexWithinRow(RowStart, RowEnd, L"Rooms", 1U);
	columnindexlookup.RawRequiredforDepts = FindCellIndexWithinRow(RowStart, RowEnd, L"Required for Dept.", 1U);
	columnindexlookup.CodeSec2	= FindCellIndexWithinRow(RowStart, RowEnd, L"Code.Sec", 2U);

	if (columnindexlookup.CodeSec == 0 || columnindexlookup.Name == 0 || columnindexlookup.Cr == 0 || columnindexlookup.Instr == 0 || columnindexlookup.Days == 0 || columnindexlookup.Hours == 0 || columnindexlookup.Rooms == 0 || columnindexlookup.CodeSec2 == 0)
	{
		fwprintf(stderr, L"%d: title row misses at least one of the required cells\n", __LINE__);
		return;
	}

	MyProgramme * PProgramme = NULL;
	MyLesson * PLesson = NULL;
	MyLookup * PSectionLookup = NULL;

	for (RowStart = wcsstradvance(RowStart, L"<tr class=\"schtd"); RowStart; RowStart = wcsstradvance(RowStart, L"<tr class=\"schtd"))
	{
		RowEnd = wcsstr(RowStart, L"</tr>");
		if (RowEnd == NULL)
		{
			fwprintf(stderr, L"%d: one row missed ending tag in %ls\n", __LINE__, PLinkLookup->Entry);
			return;
		}

		// Code.Sec2 cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.CodeSec2, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		helper = CellStart;
		while (iswalpha(helper[0]))
		{
			if (++helper >= CellEnd)
			{
				fwprintf(stderr, L"%d: Code.Sec2 cell ended too soon\n", __LINE__);
				exit(EXIT_FAILURE);
			}
		}
		PProgramme = FindTheProgramme(CellStart, helper);

		while (helper[0] == L' ')
		{
			if (++helper >= CellEnd)
			{
				fwprintf(stderr, L"%d: Code.Sec2 cell ended too soon\n", __LINE__);
				exit(EXIT_FAILURE);
			}
		}
		CellStart = helper;
		while (helper[0] != L'.')
		{
			if (++helper >= CellEnd)
			{
				fwprintf(stderr, L"%d: Code.Sec2 cell ended too soon\n", __LINE__);
				exit(EXIT_FAILURE);
			}
		}
		PLesson = FindTheLesson(CellStart, helper, &PProgramme->APLessons);

		helper++;
		if (helper >= CellEnd)
		{
			fwprintf(stderr, L"%d: Code.Sec2 cell ended too soon\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PSectionLookup = FindTheLookup(helper, CellEnd, &APSectionLookups);
		
		// Code.Sec cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.CodeSec, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		MyBonus * PBonus = NULL;
		MySection * PSection = NULL;

		if (CellStart == CellEnd)	// means that it is a bonus (ps, lab, etc.)
		{
			// Name cell
			if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Name, &CellStart, &CellEnd))
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			
			PBonus = FindTheBonus(CellStart, CellEnd, &PLesson->APBonuses);
			PSection = OneMoreSection(&PBonus->APSections);
		}
		else
		{
			// Name cell
			if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Name, &CellStart, &CellEnd))
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			PLesson->PLessonNameLookup = FindTheLookup(CellStart, CellEnd, &APLessonNameLookups);

			// Cr. cell
			if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Cr, &CellStart, &CellEnd))
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			PLesson->PCreditsLookup = FindTheLookup(CellStart, CellEnd, &APCreditsLookups);

			PLesson->PLinkLookup = PLinkLookup;
			PSection = OneMoreSection(&PLesson->APMainSections);
		}

		PSection->PSectionLookup = PSectionLookup;

		// Instr. cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Instr, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PSection->PInstructorLookup = FindTheLookup(CellStart, CellEnd, &APInstructorLookups);

		// Days cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Days, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PSection->PRawDaysLookup = FindTheLookup(CellStart, CellEnd, &APRawDaysLookups);

		// Hours cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Hours, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PSection->PRawSlotsLookup = FindTheLookup(CellStart, CellEnd, &APRawSlotsLookups);

		// Rooms cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Rooms, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PSection->PRawClassroomsLookup = FindTheLookup(CellStart, CellEnd, &APRawClassroomsLookups);

		// Required for Dept. cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.RawRequiredforDepts, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PSection->PRawRequiredforDeptsLookup = FindTheLookup(CellStart, CellEnd, &APRawRequiredforDeptsLookups);

		// parsing of raw days, slots and classrooms
		if (wcscmp(PSection->PRawDaysLookup->Entry, L"TBA") == 0)
		{
			PSection->SectionState = SectionState_TBA;
		}
		else
		{
			EvaluateRawDays(PSection);
			if (PSection->SectionState == SectionState_Failed)
			{
				fwprintf(stderr, (PBonus == NULL) ?
						 L"%d: %ls %ls.%ls%ls parsing failure on D (%ls)\n" :
						 L"%d: %ls %ls.%ls (%ls) parsing failure on D (%ls)\n", __LINE__,
						 PProgramme->PProgrammeLookup->Entry,
						 PLesson->PLessonLookup->Entry,
						 PSection->PSectionLookup->Entry,
						 PBonus == NULL ? L"" : PBonus->PBonusTypeLookup->Entry,
						 PSection->PRawDaysLookup->Entry);
				continue;
			}

			EvaluateRawSlots(PSection);
			if (PSection->SectionState == SectionState_OnlyDays)
			{
				fwprintf(stderr, (PBonus == NULL) ?
						 L"%d: %ls %ls.%ls%ls parsing failure on S (%ls), %u lectures\n" :
						 L"%d: %ls %ls.%ls (%ls) parsing failure on S (%ls), %u lectures\n", __LINE__,
						 PProgramme->PProgrammeLookup->Entry,
						 PLesson->PLessonLookup->Entry,
						 PSection->PSectionLookup->Entry,
						 PBonus == NULL ? L"" : PBonus->PBonusTypeLookup->Entry,
						 PSection->PRawSlotsLookup->Entry,
						 PSection->NumberofLectures);
				continue;
			}

			EvaluateRawClassrooms(PSection);
			if (PSection->SectionState == SectionState_DaysandSlots)
			{
				fwprintf(stderr, (PBonus == NULL) ?
						 L"%d: %ls %ls.%ls%ls parsing failure on C (%ls), %u lectures\n" :
						 L"%d: %ls %ls.%ls (%ls) parsing failure on C (%ls), %u lectures\n", __LINE__,
						 PProgramme->PProgrammeLookup->Entry,
						 PLesson->PLessonLookup->Entry,
						 PSection->PSectionLookup->Entry,
						 PBonus == NULL ? L"" : PBonus->PBonusTypeLookup->Entry,
						 PSection->PRawClassroomsLookup->Entry,
						 PSection->NumberofLectures);
				continue;
			}
		}
	}
}

void InterpretSemesterNavigationPage(const MemoryStruct * const data)
{
	const wchar_t * datacaret	= data->memory;
	const wchar_t * start		= NULL;
	const wchar_t * end			= NULL;

	const size_t BaseLinkLength = wcslen(BaseLink);

	for (datacaret = MatchFromLeftToRight(datacaret, L"href=\"", L"\"", &start, &end); datacaret; datacaret = MatchFromLeftToRight(datacaret, L"href=\"", L"\"", &start, &end))
	{
		if (wcswithstartandendstr(start, end, L"bolum="))
		{
			const size_t ADepartmentPageLinkLength = BaseLinkLength + (end - start);
			wchar_t * ADepartmentPageLink = malloc((ADepartmentPageLinkLength + 1) * sizeof * ADepartmentPageLink);
			if (ADepartmentPageLink == NULL)
			{
				fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			wcscpy(ADepartmentPageLink, BaseLink);
			wcscpywithstartandend(ADepartmentPageLink + BaseLinkLength, start, end);

			// the returned ID is not useful for anything, therefore ignored
			FindTheLookup(ADepartmentPageLink, ADepartmentPageLink + ADepartmentPageLinkLength, &APLinkLookups);

			free(ADepartmentPageLink);
		}
	}
}

// it returns a wcs which contains the link to the
// latest Semester Navigation Page
wchar_t * InterpretSemesterSelectionPage(const MemoryStruct * const data)
{
	const wchar_t * datacaret = data->memory;

	const size_t BaseLinkLength = wcslen(BaseLink);

	const wchar_t * SublinkStart = NULL;
	const wchar_t * SublinkEnd = NULL;
	datacaret = MatchFromLeftToRight(datacaret, L"<FORM ACTION=\'", L"\'", &SublinkStart, &SublinkEnd);
	if (datacaret == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	const size_t SublinkLength = SublinkEnd - SublinkStart;

	const wchar_t * InputNameStart = NULL;
	const wchar_t * InputNameEnd = NULL;
	datacaret = MatchFromLeftToRight(datacaret, L"<select name=\'", L"\'", &InputNameStart, &InputNameEnd);
	if (datacaret == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	const size_t InputNameLength = InputNameEnd - InputNameStart;

	const wchar_t * InputValueStart = NULL;
	const wchar_t * InputValueEnd = NULL;
	datacaret = MatchFromLeftToRight(datacaret, L"<option value=\'", L"\'", &InputValueStart, &InputValueEnd);
	if (datacaret == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	const size_t InputValueLength = InputValueEnd - InputValueStart;

	wchar_t * const SemesterNavigationPageLink = malloc((BaseLinkLength + SublinkLength + 1 /*?*/ + InputNameLength + 1 /*=*/ + InputValueLength + 1) * sizeof * SemesterNavigationPageLink);

	// BaseLink
	wcscpy(SemesterNavigationPageLink, BaseLink);

	// BaseLinkSublink
	wcscpywithstartandend(SemesterNavigationPageLink + BaseLinkLength, SublinkStart, SublinkEnd);

	// BaseLinkSublink?~~~~~~~~~~
	SemesterNavigationPageLink[BaseLinkLength + SublinkLength] = L'?';

	// BaseLinkSublink?InputName
	wcscpywithstartandend(SemesterNavigationPageLink + BaseLinkLength + SublinkLength + 1, InputNameStart, InputNameEnd);

	// BaseLinkSublink?InputName=~~~~~~~~~~
	SemesterNavigationPageLink[BaseLinkLength + SublinkLength + 1 + InputNameLength] = L'=';

	// BaseLinkSublink?InputName=InputValue
	wcscpywithstartandend(SemesterNavigationPageLink + BaseLinkLength + SublinkLength + 1 + InputNameLength + 1, InputValueStart, InputValueEnd);

	return SemesterNavigationPageLink;
}

size_t writefunction(const void * const delivery, const size_t size, const size_t count, MemoryStruct * const storage)
{
	storage->memory = realloc(storage->memory, (storage->count + count) * sizeof * storage->memory);
	if (storage->memory == NULL)
	{
		fwprintf(stderr, L"%d: not enough memory (realloc returned NULL)\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	mbstowcs(storage->memory + storage->count - 1, delivery, count);
	storage->count += count;
	storage->memory[storage->count - 1] = 0;

	return count;
}

void * MemoryStructReinitialize(MemoryStruct * const storage)
{
	storage->count = 1;
	storage->memory = realloc(storage->memory, sizeof * storage->memory);
	if (storage->memory)
	{
		*storage->memory = 0;
	}
	else
	{
		fwprintf(stderr, L"%d: not enough memory (realloc returned NULL)\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	return storage->memory;
}

void InitializeLookup(MyLookup *** const PAPLookup)
{
	// First entry shall be an empty string reserved as an initial value for newly created elements

	(*PAPLookup) = malloc(2 * sizeof * (*PAPLookup));
	if ((*PAPLookup) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	(*PAPLookup)[1] = NULL;

	(*PAPLookup)[0] = malloc(sizeof * (*PAPLookup)[0]);
	if ((*PAPLookup)[0] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLookup)[0]->Entry = malloc(sizeof * (*PAPLookup)[0]->Entry);
	if ((*PAPLookup)[0]->Entry == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	(*PAPLookup)[0]->Entry[0] = L'\0';

	(*PAPLookup)[0]->OrderedIndex = 0U;
}

int CompareLookups(const void * Lookup1, const void * Lookup2)
{
	wchar_t * Entry1 = (*((MyLookup **) Lookup1))->Entry;
	wchar_t * Entry2 = (*((MyLookup **) Lookup2))->Entry;
	return wcscmp(Entry1, Entry2);
}

void SortArrayofLookups(MyLookup ** const APLookups)
{
	size_t size = 0;
	while (APLookups[size] != NULL)
		size++;

	qsort(APLookups, size, sizeof * APLookups, CompareLookups);

	for (size_t i = 0; i < size; i++)
		APLookups[i]->OrderedIndex = i;
}

void InitializeGlobals(void)
{
	InitializeLookup(&APProgrammeLookups);
	InitializeLookup(&APLessonLookups);
	InitializeLookup(&APLessonNameLookups);
	InitializeLookup(&APCreditsLookups);
	InitializeLookup(&APLinkLookups);
	InitializeLookup(&APBonusTypeLookups);
	// 2, because ID = 1 will be used for main type of lectures
	InitializeLookup(&APSectionLookups);
	InitializeLookup(&APInstructorLookups);
	InitializeLookup(&APRawDaysLookups);
	InitializeLookup(&APRawSlotsLookups);
	InitializeLookup(&APRawClassroomsLookups);
	InitializeLookup(&APRawRequiredforDeptsLookups);
	InitializeLookup(&APClassroomLookups);

	APProgrammes = malloc(sizeof * APProgrammes);
	if (APProgrammes == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	APProgrammes[0] = NULL;
}

size_t FindLongestEntryLength(const MyLookup * const * PFirstLookupP)
{
	size_t LongestEntryLength = 0U;
	for (; *PFirstLookupP; PFirstLookupP++)
	{
		LongestEntryLength = max(wcslen(PFirstLookupP[0]->Entry), LongestEntryLength);
	}
	return LongestEntryLength;
}

void InitializeSomeExtremes(DAYS * const FirstDay, DAYS * const LastDay, unsigned int * const FirstSlot, unsigned int * const LastSlot)
{
	for (const MyProgramme * const * PFirstProgrammeP = APProgrammes; *PFirstProgrammeP; PFirstProgrammeP++)
	{
		for (const MyLesson * const * PFirstLessonP = (*PFirstProgrammeP)->APLessons; *PFirstLessonP; PFirstLessonP++)
		{
			for (const MySection * const * PFirstMainSectionP = (*PFirstLessonP)->APMainSections; *PFirstMainSectionP; PFirstMainSectionP++)
			{
				if ((*PFirstMainSectionP)->NumberofLectures > 0 && (*PFirstMainSectionP)->SectionState >= SectionState_DaysandSlots)
				{
					*FirstDay = PFirstMainSectionP[0]->ALectures[0].Day;
					*LastDay = PFirstMainSectionP[0]->ALectures[0].Day;
					*FirstSlot = PFirstMainSectionP[0]->ALectures[0].Slot;
					*LastSlot = PFirstMainSectionP[0]->ALectures[0].Slot;
					return;
				}
			}

			for (const MyBonus * const * PFirstBonusP = (*PFirstLessonP)->APBonuses; *PFirstBonusP; PFirstBonusP++)
			{
				for (const MySection * const * PFirstSectionP = (*PFirstBonusP)->APSections; *PFirstSectionP; PFirstSectionP++)
				{
					if ((*PFirstSectionP)->NumberofLectures > 0 && (*PFirstSectionP)->SectionState >= SectionState_DaysandSlots)
					{
						*FirstDay = PFirstSectionP[0]->ALectures[0].Day;
						*LastDay = PFirstSectionP[0]->ALectures[0].Day;
						*FirstSlot = PFirstSectionP[0]->ALectures[0].Slot;
						*LastSlot = PFirstSectionP[0]->ALectures[0].Slot;
						return;
					}
				}
			}
		}
	}
}

void PrintExtremesIntoFile(FILE * ExtremesFile)
{
	DAYS FirstDay;
	DAYS LastDay;
	unsigned int FirstSlot;
	unsigned int LastSlot;

	InitializeSomeExtremes(&FirstDay, &LastDay, &FirstSlot, &LastSlot);

	for (const MyProgramme * const * PFirstProgrammeP = APProgrammes; *PFirstProgrammeP; PFirstProgrammeP++)
	{
		for (const MyLesson * const * PFirstLessonP = (*PFirstProgrammeP)->APLessons; *PFirstLessonP; PFirstLessonP++)
		{
			for (const MySection * const * PFirstMainSectionP = (*PFirstLessonP)->APMainSections; *PFirstMainSectionP; PFirstMainSectionP++)
			{
				for (size_t LectureIndex = 0; LectureIndex < (*PFirstMainSectionP)->NumberofLectures; LectureIndex++)
				{
					if (PFirstMainSectionP[0]->ALectures[LectureIndex].Day < FirstDay)
						FirstDay = PFirstMainSectionP[0]->ALectures[LectureIndex].Day;
					if (PFirstMainSectionP[0]->ALectures[LectureIndex].Day > LastDay)
						LastDay = PFirstMainSectionP[0]->ALectures[LectureIndex].Day;
					if ((*PFirstMainSectionP)->SectionState >= SectionState_DaysandSlots)
					{
						if (PFirstMainSectionP[0]->ALectures[LectureIndex].Slot < FirstSlot)
							FirstSlot = PFirstMainSectionP[0]->ALectures[LectureIndex].Slot;
						if (PFirstMainSectionP[0]->ALectures[LectureIndex].Slot > LastSlot)
							LastSlot = PFirstMainSectionP[0]->ALectures[LectureIndex].Slot;
					}
				}
			}

			for (const MyBonus * const * PFirstBonusP = (*PFirstLessonP)->APBonuses; *PFirstBonusP; PFirstBonusP++)
			{
				for (const MySection * const * PFirstSectionP = (*PFirstBonusP)->APSections; *PFirstSectionP; PFirstSectionP++)
				{
					for (size_t LectureIndex = 0; LectureIndex < (*PFirstSectionP)->NumberofLectures; LectureIndex++)
					{
						if (PFirstSectionP[0]->ALectures[LectureIndex].Day < FirstDay)
							FirstDay = PFirstSectionP[0]->ALectures[LectureIndex].Day;
						if (PFirstSectionP[0]->ALectures[LectureIndex].Day > LastDay)
							LastDay = PFirstSectionP[0]->ALectures[LectureIndex].Day;
						if ((*PFirstSectionP)->SectionState >= SectionState_DaysandSlots)
						{
							if (PFirstSectionP[0]->ALectures[LectureIndex].Slot < FirstSlot)
								FirstSlot = PFirstSectionP[0]->ALectures[LectureIndex].Slot;
							if (PFirstSectionP[0]->ALectures[LectureIndex].Slot > LastSlot)
								LastSlot = PFirstSectionP[0]->ALectures[LectureIndex].Slot;
						}
					}
				}
			}
		}
	}

	fwprintf(ExtremesFile, L"%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u",
			 (unsigned int) FirstDay,
			 (unsigned int) LastDay,
			 FirstSlot,
			 LastSlot,
			 FindLongestEntryLength(APProgrammeLookups),
			 FindLongestEntryLength(APLessonLookups),
			 FindLongestEntryLength(APLessonNameLookups),
			 FindLongestEntryLength(APCreditsLookups),
			 FindLongestEntryLength(APLinkLookups),
			 FindLongestEntryLength(APBonusTypeLookups),
			 FindLongestEntryLength(APSectionLookups),
			 FindLongestEntryLength(APInstructorLookups),
			 FindLongestEntryLength(APRawDaysLookups),
			 FindLongestEntryLength(APRawSlotsLookups),
			 FindLongestEntryLength(APRawClassroomsLookups),
			 FindLongestEntryLength(APRawRequiredforDeptsLookups),
			 FindLongestEntryLength(APClassroomLookups)
			 );
}

int main(void)
{
	time_t TimeStart = time(NULL);
	clock_t ClockStart = clock( );

	InitializeGlobals( );
	setlocale(LC_ALL, "tr-TR");

	FILE * ErrorLog = _wfreopen(L"error.log", L"w, ccs=UNICODE", stderr);
	if (ErrorLog == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
	}

	CURL * curl;
	CURLcode res;
	char URL[2048];

	MemoryStruct storage = {
		0,
		NULL
	};
	MemoryStructReinitialize(&storage);

	if (curl_global_init(CURL_GLOBAL_ALL))
	{
		fwprintf(stderr, L"%d: something went wrong with curl_global_init( )\n", __LINE__);
		return EXIT_FAILURE;
	}

	curl = curl_easy_init( );
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &storage);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunction);


		// Semester Selection Page: the one with drop-down list to select semester
		URL[wcstombs(URL, SemesterSelectionPageLink, sizeof URL / sizeof * URL - 1)] = 0;
		curl_easy_setopt(curl, CURLOPT_URL, URL);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fwprintf(stderr, L"%d: curl_easy_perform( ) failed: %hs\n", __LINE__, curl_easy_strerror(res));
			return EXIT_FAILURE;
		}

		wchar_t * SemesterNavigationPageLink = InterpretSemesterSelectionPage(&storage);
		MemoryStructReinitialize(&storage);
		wprintf(L"%ls\n", SemesterNavigationPageLink);


		// Semester Navigation Page: the one with 2-column table full with links
		URL[wcstombs(URL, SemesterNavigationPageLink, sizeof URL / sizeof * URL - 1)] = 0;
		free(SemesterNavigationPageLink);
		SemesterNavigationPageLink = NULL;
		curl_easy_setopt(curl, CURLOPT_URL, URL);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fwprintf(stderr, L"%d: curl_easy_perform( ) failed: %hs\n", __LINE__, curl_easy_strerror(res));
			return EXIT_FAILURE;
		}

		InterpretSemesterNavigationPage(&storage);
		MemoryStructReinitialize(&storage);

		// LookupID = 0 is reserved as a default value for structure elements
		for (size_t LinkIndex = 1; APLinkLookups[LinkIndex]; LinkIndex++)
		{
			URL[wcstombs(URL, APLinkLookups[LinkIndex]->Entry, sizeof URL / sizeof * URL - 1)] = 0;
			curl_easy_setopt(curl, CURLOPT_URL, URL);
			res = curl_easy_perform(curl);
			if (res != CURLE_OK)
			{
				fwprintf(stderr, L"%d: curl_easy_perform( ) failed: %hs\n", __LINE__, curl_easy_strerror(res));
				return EXIT_FAILURE;
			}

			InterpretDepartmentPage(&storage, APLinkLookups[LinkIndex]);
			MemoryStructReinitialize(&storage);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	else
	{
		fwprintf(stderr, L"%d: something went wrong with curl_easy_init( )\n", __LINE__);
		return EXIT_FAILURE;
	}

	SortArrayofLookups(APProgrammeLookups);
	SortArrayofLookups(APLessonLookups);
	SortArrayofLookups(APLessonNameLookups);
	SortArrayofLookups(APCreditsLookups);
	SortArrayofLookups(APLinkLookups);
	SortArrayofLookups(APBonusTypeLookups);
	SortArrayofLookups(APSectionLookups);
	SortArrayofLookups(APInstructorLookups);
	SortArrayofLookups(APRawDaysLookups);
	SortArrayofLookups(APRawSlotsLookups);
	SortArrayofLookups(APRawClassroomsLookups);
	SortArrayofLookups(APRawRequiredforDeptsLookups);
	SortArrayofLookups(APClassroomLookups);

	FILE * ProgrammeLookupFile				= _wfopen(L"ProgrammeLookup.txt",			L"w, ccs=UNICODE");
	FILE * LessonLookupFile					= _wfopen(L"LessonLookup.txt",				L"w, ccs=UNICODE");
	FILE * LessonNameLookupFile				= _wfopen(L"LessonNameLookup.txt",			L"w, ccs=UNICODE");
	FILE * CreditsLookupFile				= _wfopen(L"CreditsLookup.txt",				L"w, ccs=UNICODE");
	FILE * LinkLookupFile					= _wfopen(L"LinkLookup.txt",				L"w, ccs=UNICODE");
	FILE * BonusTypeLookupFile				= _wfopen(L"BonusTypeLookup.txt",			L"w, ccs=UNICODE");
	FILE * SectionLookupFile				= _wfopen(L"SectionLookup.txt",				L"w, ccs=UNICODE");
	FILE * InstructorLookupFile				= _wfopen(L"InstructorLookup.txt",			L"w, ccs=UNICODE");
	FILE * RawDaysLookupFile				= _wfopen(L"RawDaysLookup.txt",				L"w, ccs=UNICODE");
	FILE * RawSlotsLookupFile				= _wfopen(L"RawSlotsLookup.txt",			L"w, ccs=UNICODE");
	FILE * RawClassroomsLookupFile			= _wfopen(L"RawClassroomsLookup.txt",		L"w, ccs=UNICODE");
	FILE * RawRequiredforDeptsLookupFile	= _wfopen(L"RawRequiredforDeptsLookup.txt",	L"w, ccs=UNICODE");
	FILE * ClassroomLookupFile				= _wfopen(L"ClassroomLookup.txt",			L"w, ccs=UNICODE");

	FILE * ProgrammesArrayFile				= _wfopen(L"ProgrammesArray.txt",			L"w, ccs=UNICODE");
	FILE * ExtremesFile						= _wfopen(L"Extremes.txt",					L"w, ccs=UNICODE");
	if (!ProgrammeLookupFile || !LessonLookupFile || !LessonNameLookupFile || !CreditsLookupFile || !LinkLookupFile || !BonusTypeLookupFile || !SectionLookupFile ||
		!InstructorLookupFile || !RawDaysLookupFile || !RawSlotsLookupFile || !RawClassroomsLookupFile || !RawRequiredforDeptsLookupFile || !ClassroomLookupFile || !ProgrammesArrayFile || !ExtremesFile)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		return EXIT_FAILURE;
	}

	PrintLookupIntoFile(ProgrammeLookupFile, APProgrammeLookups);
	fclose(ProgrammeLookupFile);
	ProgrammeLookupFile = NULL;

	PrintLookupIntoFile(LessonLookupFile, APLessonLookups);
	fclose(LessonLookupFile);
	LessonLookupFile = NULL;

	PrintLookupIntoFile(LessonNameLookupFile, APLessonNameLookups);
	fclose(LessonNameLookupFile);
	LessonNameLookupFile = NULL;

	PrintLookupIntoFile(CreditsLookupFile, APCreditsLookups);
	fclose(CreditsLookupFile);
	CreditsLookupFile = NULL;

	PrintLookupIntoFile(LinkLookupFile, APLinkLookups);
	fclose(LinkLookupFile);
	LinkLookupFile = NULL;

	PrintLookupIntoFile(BonusTypeLookupFile, APBonusTypeLookups);
	fclose(BonusTypeLookupFile);
	BonusTypeLookupFile = NULL;

	PrintLookupIntoFile(SectionLookupFile, APSectionLookups);
	fclose(SectionLookupFile);
	SectionLookupFile = NULL;

	PrintLookupIntoFile(InstructorLookupFile, APInstructorLookups);
	fclose(InstructorLookupFile);
	InstructorLookupFile = NULL;

	PrintLookupIntoFile(RawDaysLookupFile, APRawDaysLookups);
	fclose(RawDaysLookupFile);
	RawDaysLookupFile = NULL;

	PrintLookupIntoFile(RawSlotsLookupFile, APRawSlotsLookups);
	fclose(RawSlotsLookupFile);
	RawSlotsLookupFile = NULL;

	PrintLookupIntoFile(RawClassroomsLookupFile, APRawClassroomsLookups);
	fclose(RawClassroomsLookupFile);
	RawClassroomsLookupFile = NULL;

	PrintLookupIntoFile(RawRequiredforDeptsLookupFile, APRawRequiredforDeptsLookups);
	fclose(RawRequiredforDeptsLookupFile);
	RawRequiredforDeptsLookupFile = NULL;

	PrintLookupIntoFile(ClassroomLookupFile, APClassroomLookups);
	fclose(ClassroomLookupFile);
	ClassroomLookupFile = NULL;

	PrintProgrammesIntoFile(ProgrammesArrayFile);
	fclose(ProgrammesArrayFile);
	ProgrammesArrayFile = NULL;

	PrintExtremesIntoFile(ExtremesFile);
	fclose(ExtremesFile);
	ExtremesFile = NULL;

	FILE * UpdateInformationFile = _wfopen(L"UpdateInformation.txt", L"w, ccs=UNICODE");
	if (!UpdateInformationFile)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		return EXIT_FAILURE;
	}

	clock_t ClockEnd = clock( );
	wchar_t * UpdateTime = _wasctime(localtime(&TimeStart));
	fwprintf(UpdateInformationFile, L"%.*ls\"%ld", wcslen(UpdateTime) - 1, UpdateTime, ClockEnd - ClockStart);

	fclose(UpdateInformationFile);
	UpdateInformationFile = NULL;

	wprintf(L"execution has been completed in %ld milliseconds\n", ClockEnd - ClockStart);
	fwprintf(stderr, L"%d: execution has been completed in %ld milliseconds\n", __LINE__, ClockEnd - ClockStart);

	if (ErrorLog)
	{
		fclose(ErrorLog);
		ErrorLog = NULL;
	}

	return 0;
}