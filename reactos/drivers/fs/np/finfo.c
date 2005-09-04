/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/fs/np/finfo.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include <debug.h>

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

static
NTSTATUS
NpfsSetPipeInformation(PDEVICE_OBJECT DeviceObject,
                       PNPFS_FCB Fcb,
                       PFILE_PIPE_INFORMATION Info,
                       PULONG BufferLength)
{
    PNPFS_PIPE Pipe;
    PFILE_PIPE_INFORMATION Request;
    DPRINT("NpfsSetPipeInformation()\n");

    /* Get the Pipe and data */
    Pipe = Fcb->Pipe;
    Request = (PFILE_PIPE_INFORMATION)Info;
    
    /* Set Pipe Data */
    Pipe->ReadMode = Request->ReadMode;
    Pipe->CompletionMode =  Request->CompletionMode;

    /* Return Success */
    return STATUS_SUCCESS;  
}

static
NTSTATUS
NpfsSetPipeRemoteInformation(PDEVICE_OBJECT DeviceObject,
                             PNPFS_FCB Fcb,
                             PFILE_PIPE_INFORMATION Info,
                             PULONG BufferLength)
{
    PNPFS_PIPE Pipe;
    PFILE_PIPE_REMOTE_INFORMATION Request;
    DPRINT("NpfsSetPipeRemoteInformation()\n");

    /* Get the Pipe and data */
    Pipe = Fcb->Pipe;
    Request = (PFILE_PIPE_REMOTE_INFORMATION)Info;
    
    /* Set the Settings */
    Pipe->TimeOut = Request->CollectDataTime;
    Pipe->InboundQuota = Request->MaximumCollectionCount;
    
    /* Return Success */
    return STATUS_SUCCESS;
}
      
static 
NTSTATUS
NpfsQueryPipeInformation(PDEVICE_OBJECT DeviceObject,
                         PNPFS_FCB Fcb,
                         PFILE_PIPE_INFORMATION Info,
                         PULONG BufferLength)
{
    PNPFS_PIPE Pipe;
    DPRINT("NpfsQueryPipeInformation()\n");

    /* Get the Pipe */
    Pipe = Fcb->Pipe;
    
    /* Clear Info */
    RtlZeroMemory(Info, sizeof(FILE_PIPE_INFORMATION));

    /* Return Info */
    Info->CompletionMode = Pipe->CompletionMode;
    Info->ReadMode = Pipe->ReadMode;
     
    /* Return success */
    *BufferLength -= sizeof(FILE_PIPE_INFORMATION);
    return STATUS_SUCCESS;
}

static 
NTSTATUS
NpfsQueryPipeRemoteInformation(PDEVICE_OBJECT DeviceObject,
                               PNPFS_FCB Fcb,
                               PFILE_PIPE_REMOTE_INFORMATION Info,
                               PULONG BufferLength)
{
    PNPFS_PIPE Pipe;
    DPRINT("NpfsQueryPipeRemoteInformation()\n");

    /* Get the Pipe */
    Pipe = Fcb->Pipe;
    
    /* Clear Info */
    RtlZeroMemory(Info, sizeof(FILE_PIPE_REMOTE_INFORMATION));

    /* Return Info */
    Info->MaximumCollectionCount = Pipe->InboundQuota;
    Info->CollectDataTime = Pipe->TimeOut;
     
    /* Return success */
    *BufferLength -= sizeof(FILE_PIPE_REMOTE_INFORMATION);
    return STATUS_SUCCESS;
}


static NTSTATUS
NpfsQueryLocalPipeInformation(PDEVICE_OBJECT DeviceObject,
			      PNPFS_FCB Fcb,
			      PFILE_PIPE_LOCAL_INFORMATION Info,
			      PULONG BufferLength)
{
  PNPFS_PIPE Pipe;

  DPRINT("NpfsQueryLocalPipeInformation()\n");

  Pipe = Fcb->Pipe;

  RtlZeroMemory(Info,
		sizeof(FILE_PIPE_LOCAL_INFORMATION));

  Info->NamedPipeType = Pipe->PipeType;
  Info->NamedPipeConfiguration = Pipe->PipeConfiguration;
  Info->MaximumInstances = Pipe->MaximumInstances;
  Info->CurrentInstances = Pipe->CurrentInstances;
  Info->InboundQuota = Pipe->InboundQuota;
  Info->OutboundQuota = Pipe->OutboundQuota;
  Info->NamedPipeState = Fcb->PipeState;
  Info->NamedPipeEnd = Fcb->PipeEnd;

  if (Fcb->PipeEnd == FILE_PIPE_SERVER_END)
    {
      Info->ReadDataAvailable = Fcb->ReadDataAvailable;
      Info->WriteQuotaAvailable = Fcb->WriteQuotaAvailable;
    }
  else if (Fcb->OtherSide != NULL)
    {
      Info->ReadDataAvailable = Fcb->OtherSide->ReadDataAvailable;
      Info->WriteQuotaAvailable = Fcb->OtherSide->WriteQuotaAvailable;
    }

  *BufferLength -= sizeof(FILE_PIPE_LOCAL_INFORMATION);
  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NpfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  FILE_INFORMATION_CLASS FileInformationClass;
  PFILE_OBJECT FileObject;
  PNPFS_DEVICE_EXTENSION DeviceExtension;
  PNPFS_FCB Fcb;
  PNPFS_PIPE Pipe;
  PVOID SystemBuffer;
  ULONG BufferLength;
  NTSTATUS Status;

  DPRINT("NpfsQueryInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

  IoStack = IoGetCurrentIrpStackLocation (Irp);
  FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
  DeviceExtension = DeviceObject->DeviceExtension;
  FileObject = IoStack->FileObject;
  Fcb = (PNPFS_FCB)FileObject->FsContext;
  Pipe = Fcb->Pipe;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  BufferLength = IoStack->Parameters.QueryFile.Length;

  DPRINT("Pipe name: %wZ\n", &Pipe->PipeName);
  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("SystemBuffer %p\n", SystemBuffer);
  DPRINT("BufferLength %lu\n", BufferLength);

  switch (FileInformationClass)
    {
      case FilePipeInformation:
	Status = NpfsQueryPipeInformation(DeviceObject,
					  Fcb,
					  SystemBuffer,
					  &BufferLength);
	break;

      case FilePipeLocalInformation:
	Status = NpfsQueryLocalPipeInformation(DeviceObject,
					       Fcb,
					       SystemBuffer,
					       &BufferLength);
	break;

      case FilePipeRemoteInformation:
	Status = NpfsQueryPipeRemoteInformation(DeviceObject,
					       Fcb,
					       SystemBuffer,
					       &BufferLength);
	break;

      default:
	Status = STATUS_NOT_SUPPORTED;
    }

  Irp->IoStatus.Status = Status;
  if (NT_SUCCESS(Status))
    Irp->IoStatus.Information =
      IoStack->Parameters.QueryFile.Length - BufferLength;
  else
    Irp->IoStatus.Information = 0;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return Status;
}


NTSTATUS STDCALL
NpfsSetInformation(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  FILE_INFORMATION_CLASS FileInformationClass;
  PFILE_OBJECT FileObject;
  PNPFS_FCB Fcb;
  PNPFS_PIPE Pipe;
  PVOID SystemBuffer;
  ULONG BufferLength;
  NTSTATUS Status;

  DPRINT("NpfsSetInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

  IoStack = IoGetCurrentIrpStackLocation (Irp);
  FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
  FileObject = IoStack->FileObject;
  Fcb = (PNPFS_FCB)FileObject->FsContext;
  Pipe = Fcb->Pipe;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  BufferLength = IoStack->Parameters.QueryFile.Length;

  DPRINT("Pipe name: %wZ\n", &Pipe->PipeName);
  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("SystemBuffer %p\n", SystemBuffer);
  DPRINT("BufferLength %lu\n", BufferLength);

    switch (FileInformationClass)
    {
        case FilePipeInformation:
            /* Call the handler */
            Status = NpfsSetPipeInformation(DeviceObject,
                                            Fcb,
                                            SystemBuffer,
                                            &BufferLength);
        break;
      
        case FilePipeLocalInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
            
        case FilePipeRemoteInformation:
            /* Call the handler */
            Status = NpfsSetPipeRemoteInformation(DeviceObject,
                                                  Fcb,
                                                  SystemBuffer,
                                                  &BufferLength);
        break;
            default:
            Status = STATUS_NOT_SUPPORTED;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/* EOF */
