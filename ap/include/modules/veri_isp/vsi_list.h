/*****************************************************************************
 * Copyright (c) 2024 by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")
 * All Rights Reserved.
 *
 * The material in this file is confidential and contains trade secrets of
 * of VeriSilicon.  This is proprietary information owned or licensed by
 * VeriSilicon.  No part of this work may be disclosed, reproduced, copied,
 * transmitted, or used in any way for any purpose, without the express
 * written permission of VeriSilicon.
 *
 *****************************************************************************/

#ifndef __VSI_LIST_H__
#define __VSI_LIST_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include <stdlib.h>

struct ListHead_s {
    struct ListHead_s *Prev;
    struct ListHead_s *Next;
};

#define ContainerOf(Ptr, Type, Member) ((Type *) (((char *)Ptr) - (size_t)(&((Type *) 0)->Member)))

static inline void InitListHead(struct ListHead_s *List)
{
    List->Prev = List;
    List->Next = List;
}

static inline void __ListAdd(struct ListHead_s *New,
                    struct ListHead_s *Prev,
                    struct ListHead_s *Next)
{
    Next->Prev = New;
    New->Next = Next;
    New->Prev = Prev;
    Prev->Next = New;
}

static inline void ListAdd(struct ListHead_s *New, struct ListHead_s *Head)
{
    __ListAdd(New, Head, Head->Next);
}

static inline void ListAddTail(struct ListHead_s *New, struct ListHead_s *Head)
{
    __ListAdd(New, Head->Prev, Head);
}

static inline void __ListDel(struct ListHead_s *Prev, struct ListHead_s *Next)
{
    Next->Prev = Prev;
    Prev->Next = Next;
}

static inline void __ListDelEntry(struct ListHead_s *Entry)
{
    __ListDel(Entry->Prev, Entry->Next);
}

static inline void ListDel(struct ListHead_s *Entry)
{
    __ListDelEntry(Entry);
    Entry->Next = NULL;
    Entry->Prev = NULL;
}

static inline int ListEmpty(const struct ListHead_s *Head)
{
    return Head->Next == Head;
}

#define ListEntry(Ptr, Type, Member)                             \
        ContainerOf(Ptr, Type, Member)

#define ListFirstEntry(Ptr, Type, Member)                        \
        ListEntry((Ptr)->Next, Type, Member)

#define ListLastEntry(Ptr, Type, Member)                         \
        ListEntry((Ptr)->Prev, Type, Member)

#define ListNextEntry(Pos, Member)                               \
        ListEntry((Pos)->Member.Next, typeof(*(Pos)), Member)

#define ListPrivEntry(Pos, Member)                               \
        ListEntry((Pos)->Member.Prev, typeof(*(Pos)), Member)

#define ListForEachEntry(Pos, Head, Member)                      \
        for (Pos = ListFirstEntry(Head, typeof(*Pos), Member);   \
             &Pos->Member != (Head);                             \
             Pos = ListNextEntry(Pos, Member))

#define ListForEachEntrySafe(Pos, N, Head, Member)               \
        for (Pos = ListFirstEntry(Head, typeof(*Pos), Member),   \
                N = ListNextEntry(Pos, Member);                  \
             &Pos->Member != (Head);                             \
             Pos = N, N = ListNextEntry(N, Member))

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
